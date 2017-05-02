//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2017 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "online/steam.hpp"

#include "utils/log.hpp"
#include "utils/string_utils.hpp"

#ifdef WIN32
#  include <windows.h> 
#else
#  include <iostream>
#  include <unistd.h>
#endif

Steam * Steam::m_steam = NULL;

// ----------------------------------------------------------------------------
Steam::Steam()
{
    m_steam_available = false;

    // Create the child process SSM to mamage steam:
    if (!createChildProcess())
    {
        Log::error("Steam", "Could not start ssm.exe");
        return;
    }

    Log::info("Steam", "Starting steam manager");
        
    std::string s = sendCommand("init");
    if (s != "1")
    {
        Log::error("Steam", "Could not initialise Steam API.");
        return;
    }

    s = sendCommand("name");
    m_user_name   =  decodeString(s);
    if (m_user_name == "")
    {
        Log::error("Steam", "Can not get Steam user name.");
        return;
    }

    m_user_name_wchar = StringUtils::utf8ToWide(m_user_name);

    s = sendCommand("id");
    m_steam_id = decodeString(s);
    if (m_steam_id== "")
    {
        Log::error("Steam", "Can not get Steam id.");
        return;
    }

    m_steam_available = true;
}   // Steam

// ----------------------------------------------------------------------------
/** Terminates the child processes and shuts down the Steam API.
 */
Steam::~Steam()
{
    std::string s = sendCommand("quit");
    if (s != "quit")
    {
        Log::error("Steam", "Could not shutdown Steam process properly");
    }

#ifdef LINUX
    close(m_child_stdin_write);
    close(m_child_stdout_read);
#endif
    Log::info("Steam", "Shutting down steam manager");

}   // ~Steam

// ----------------------------------------------------------------------------
/** Starts ssm.exe (on windows) or ssm (other platforms) as a child process
 *  and sets up communication via pipes.
 *  \return True if the child process creation was successful.
 */
#ifdef WIN32
bool Steam::createChildProcess()
{
    // Based on: https://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx
    SECURITY_ATTRIBUTES sec_attr;

    // Set the bInheritHandle flag so pipe handles are inherited. 
    sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec_attr.bInheritHandle       = TRUE;
    sec_attr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 

    if (!CreatePipe(&m_child_stdout_read, &m_child_stdout_write, &sec_attr, 0))
    {
        Log::error("Steam", "Error creating StdoutRd CreatePipe");
        return;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(m_child_stdout_read, HANDLE_FLAG_INHERIT, 0))
    {
        Log::error("Steam", "Stdout SetHandleInformation");
        return;
    }

    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&m_child_stdin_read, &m_child_stdin_write, &sec_attr, 0))
    {
        Log::error("Steam", "Stdin CreatePipe");
        return;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if (!SetHandleInformation(m_child_stdin_write, HANDLE_FLAG_INHERIT, 0))
    {
        Log::error("Steam", "Stdin SetHandleInformation");
        return;
    }

    TCHAR command_line[] = TEXT("ssm.exe 1");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    // Set up members of the PROCESS_INFORMATION structure. 

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = m_child_stdout_write;
    siStartInfo.hStdOutput = m_child_stdout_write;
    siStartInfo.hStdInput = m_child_stdin_read;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process. 

    bool success = CreateProcess(NULL,
        command_line,  // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo) != 0; // receives PROCESS_INFORMATION 
       
    if (!success)
    {
        return false;
    }

    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example. 

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return true;
}   // createChildProcess - windows version

#else    // linux and osx

bool Steam::createChildProcess()
{
    const int PIPE_READ=0;
    const int PIPE_WRITE=1;
    int       stdin_pipe[2];
    int       stdout_pipe[2];
    int       child;

    if (pipe(stdin_pipe) < 0)
    {
        Log::error("Steam", "Can't allocate pipe for input redirection.");
        return -1;
    }
    if (pipe(stdout_pipe) < 0)
    {
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);
        Log::error("Steam", "allocating pipe for child output redirect");
        return false;
    }

    child = fork();
    if (child == 0)
    {
        // Child process:
        Log::info("Steam", "Child process started.");

        // redirect stdin
        if (dup2(stdin_pipe[PIPE_READ], STDIN_FILENO) == -1)
        {
            Log::error("Steam", "Redirecting stdin");
            return false;
        }

        // redirect stdout
        if (dup2(stdout_pipe[PIPE_WRITE], STDOUT_FILENO) == -1)
        {
            Log::error("Steam", "Redirecting stdout");
            return false;
        }

        // all these are for use by parent only
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);
        close(stdout_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]); 
        
        // run child process image
        execl("./ssm", "", NULL);
        Log::error("Steam", "Error in execl: errnp %d", errno);

        // if we get here at all, an error occurred, but we are in the child
        // process, so just exit
        perror("Steam: execl error");
        exit(-1);
    }
    else if (child > 0)
    {
        // parent continues here
        // close unused file descriptors, these are for child only
        close(stdin_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]); 
        m_child_stdin_write = stdin_pipe[PIPE_WRITE];
        m_child_stdout_read = stdout_pipe[PIPE_READ];
    } 
    else   // child < 0 
    {
        // failed to create child
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);
        close(stdout_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]);
        return false;
    }
    return true;
}   // createChildProcess
#endif

// ----------------------------------------------------------------------------
/** Reads a command from the input pipe.
 */
std::string Steam::getLine()
{
#define BUFSIZE 1024
    char buffer[BUFSIZE];

#ifdef WIN32
    DWORD bytes_read;
    // Read from pipe that is the standard output for child process. 
    bool success = ReadFile(m_child_stdout_read, buffer, BUFSIZE-1,
                            &bytes_read, NULL)!=0;
    if (success && bytes_read < BUFSIZE)
    {
        buffer[bytes_read] = 0;
        std::string s = buffer;
        return s;
    }
#else
    //std::string s;
    //std::getline(std::cin, s);
    //return s;

    int bytes_read = read(m_child_stdout_read, buffer, BUFSIZE-1);
    if(bytes_read>0)
    {
        buffer[bytes_read] = 0;
        std::string s = buffer;
        return s;
    }
#endif
    return std::string("");
}   // getLine

// ----------------------------------------------------------------------------
/** Sends a command to the SSM via a pipe, and reads the answer.
 *  \return Answer from SSM.
 */
std::string Steam::sendCommand(const std::string &command)
{
#ifdef WIN32
    // Write to the pipe that is the standard input for a child process. 
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data.
    DWORD bytes_written;
    bool success = WriteFile(m_child_stdin_write, (command+"\n").c_str(),
                             command.size()+1, &bytes_written, NULL     ) != 0;
#else
    write(m_child_stdin_write, (command+"\n").c_str(), command.size()+1);
#endif
    return getLine();

    return std::string("");
}   // sendCommand

// ----------------------------------------------------------------------------
/** All answer strings from 'SSM' are in the form: "length string", i.e. the
 *  length of the string, followed by a space and then the actual strings.
 *  This allows for checking on some potential problems (e.g. if a pipe should
 *  only send part of the answer string - todo: handle this problem instead of
 *  ignoring it.
 */
std::string Steam::decodeString(const std::string &s)
{
    std::vector<std::string> l = StringUtils::split(s, ' ');
    if (l.size() != 2) return "INVALID ANSWER - wrong number of fields";

    int n;
    StringUtils::fromString(l[0], n);
    if (n != (int)l[1].size()) return "INVALID ANSWER - incorrect length";

    return l[1];

}   // decodeString

// ----------------------------------------------------------------------------
/** Returns the steam user name. SSM returns 'N name" where N is
 *  the length of the name.
 */
const std::string& Steam::getUserName()
{
    assert(m_steam_available);
    return m_user_name;
}   // getUserName

// ----------------------------------------------------------------------------
/** Returns a unique id (string) from steam. SSM returns 'N ID" where N is
 *  the length of the ID.
 */
const std::string& Steam::getSteamID()
{
    assert(m_steam_available);
    return m_steam_id;
}   // getSteamID

// ----------------------------------------------------------------------------
/** Returns a std::vector with the names of all friends. SSM returns a first
 *  line with the number of friends, then one friend in a line.
 */
std::vector<std::string> Steam::getFriends()
{
    std::string s = sendCommand("friends");
    int num_friends;
    StringUtils::fromString(s, num_friends);
    std::vector<std::string> result;
    for (int i = 0; i < num_friends; i++)
    {
        std::string f = getLine();
        result.push_back(decodeString(f));
    }
    return result;
}
// ----------------------------------------------------------------------------
/** Instructs the SSM to save the avatar of the user with the specified 
 *  filename. Note that the avatar is always saved in png format (independent
 *  on what is specified as filename).
 */
int Steam::saveAvatarAs(const std::string &filename)
{
    std::string s = sendCommand("avatar");
    if(s=="filename")
        s=sendCommand(filename);
    return s == "done";

}   // saveAvatarAs
