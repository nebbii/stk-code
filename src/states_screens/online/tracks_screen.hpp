//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2015 Marianne Gagnon
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

#ifndef HEADER_TRACKS_SCREEN_HPP
#define HEADER_TRACKS_SCREEN_HPP

#include "guiengine/screen.hpp"
#include "utils/synchronised.hpp"
#include <deque>
#include <limits>
#include <map>
#include <string>

class Track;

namespace GUIEngine
{
    class CheckBoxWidget;
    class LabelWidget;
    class SpinnerWidget;
}

/**
  * \brief screen where the user can select a track
  * \ingroup states_screens
  */
class TracksScreen : public GUIEngine::Screen,
                     public GUIEngine::ScreenSingleton<TracksScreen>
{
    friend class GUIEngine::ScreenSingleton<TracksScreen>;

private:
    TracksScreen() : Screen("tracks.stkgui")
    {
        m_network_tracks = false;
        m_reverse_checked = false;
        m_quit_server = false;
        m_bottom_box_height = -1;
        m_vote_timeout = std::numeric_limits<uint64_t>::max();
    }

    Track* m_selected_track = NULL;
    GUIEngine::CheckBoxWidget* m_reversed;
    GUIEngine::SpinnerWidget* m_laps;
    GUIEngine::LabelWidget* m_votes;

    bool m_network_tracks, m_reverse_checked, m_quit_server;

    int m_bottom_box_height;

    uint64_t m_vote_timeout;

    std::map<std::string, core::stringw> m_vote_messages;

    std::deque<std::string> m_random_track_list;

    /** adds the tracks from the current track group into the tracks ribbon */
    void buildTrackList();

    void voteForPlayer();

public:

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void loadedFromFile() OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void eventCallback(GUIEngine::Widget* widget,
                               const std::string& name,
                               const int playerID) OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void init() OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void beforeAddingWidget() OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void tearDown() OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual bool onEscapePressed() OVERRIDE;

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void onUpdate(float dt) OVERRIDE;

    void setFocusOnTrack(const std::string& trackName);

    void setNetworkTracks() { m_network_tracks = true; }

    void setQuitServer() { m_quit_server = true; }

    void resetVote()
    {
        m_vote_messages.clear();
        m_vote_timeout = std::numeric_limits<uint64_t>::max();
    }

    void setVoteTimeout(float timeout);

    void addVoteMessage(const std::string& user_host_id,
                        const irr::core::stringw& message)
    {
        m_vote_messages[user_host_id] = message;
    }
    void removeVote(const std::string& user_host_id)
    {
        m_vote_messages.erase(user_host_id);
    }

};

#endif
