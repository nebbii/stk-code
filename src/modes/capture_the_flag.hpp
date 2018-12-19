//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2018 SuperTuxKart-Team
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

#ifndef CAPTURE_THE_FLAG_HPP
#define CAPTURE_THE_FLAG_HPP

#include "modes/free_for_all.hpp"

#include <vector>
#include <string>

namespace irr
{
    namespace scene
    {
        class IAnimatedMeshSceneNode; class IAnimatedMesh; class ISceneNode;
    }
}

class CaptureTheFlag : public FreeForAll
{
private:
    scene::IAnimatedMeshSceneNode* m_red_flag_node;

    scene::IAnimatedMeshSceneNode* m_blue_flag_node;

    scene::IAnimatedMesh* m_red_flag_mesh;

    scene::IAnimatedMesh* m_blue_flag_mesh;

    scene::ISceneNode* m_red_flag_indicator;

    scene::ISceneNode* m_blue_flag_indicator;

    SFXBase* m_scored_sound;

    int m_red_scores, m_blue_scores, m_red_holder, m_blue_holder;

    btTransform m_red_trans, m_blue_trans, m_orig_red_trans, m_orig_blue_trans;

    int m_red_return_ticks, m_blue_return_ticks;

    std::map<int, int> m_swatter_reset_kart_ticks;

    // ------------------------------------------------------------------------
    void updateFlagNodes();
    // ------------------------------------------------------------------------
    bool getDroppedFlagTrans(const btTransform& kt, btTransform* out) const;
    // ------------------------------------------------------------------------
    void resetRedFlagToOrigin();
    // ------------------------------------------------------------------------
    void resetBlueFlagToOrigin();
    // ------------------------------------------------------------------------
    virtual video::SColor getColor(unsigned int kart_id) const OVERRIDE;

public:
    // ------------------------------------------------------------------------
    CaptureTheFlag();
    // ------------------------------------------------------------------------
    virtual ~CaptureTheFlag();
    // ------------------------------------------------------------------------
    virtual void init() OVERRIDE;
    // ------------------------------------------------------------------------
    virtual void reset(bool restart=false) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual void update(int ticks) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual void updateGraphics(float dt) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual bool hasTeam() const OVERRIDE                      { return true; }
    // ------------------------------------------------------------------------
    virtual bool isRaceOver() OVERRIDE;
    // ------------------------------------------------------------------------
    virtual bool kartHit(int kart_id, int hitter = -1) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual unsigned int getRescuePositionIndex(AbstractKart *kart) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual const std::string& getIdent() const OVERRIDE;
    // ------------------------------------------------------------------------
    void attachFlag(NetworkString& ns);
    // ------------------------------------------------------------------------
    void resetFlag(NetworkString& ns);
    // ------------------------------------------------------------------------
    bool getKartCTFResult(unsigned int kart_id) const
    {
        if (m_red_scores == m_blue_scores)
            return true;

        bool red_win = m_red_scores > m_blue_scores;
        KartTeam team = getKartTeam(kart_id);

        if ((red_win && team == KART_TEAM_RED) ||
            (!red_win && team == KART_TEAM_BLUE))
            return true;
        else
            return false;
    }
    // ------------------------------------------------------------------------
    int getRedScore() const                            { return m_red_scores; }
    // ------------------------------------------------------------------------
    int getBlueScore() const                          { return m_blue_scores; }
    // ------------------------------------------------------------------------
    int getRedHolder() const                           { return m_red_holder; }
    // ------------------------------------------------------------------------
    int getBlueHolder() const                         { return m_blue_holder; }
    // ------------------------------------------------------------------------
    bool isRedFlagInBase() const
    {
        return m_red_holder == -1 &&
            m_red_trans.getOrigin() == m_orig_red_trans.getOrigin();
    }
    // ------------------------------------------------------------------------
    bool isBlueFlagInBase() const
    {
        return m_blue_holder == -1 &&
            m_blue_trans.getOrigin() == m_orig_blue_trans.getOrigin();
    }
    // ------------------------------------------------------------------------
    const Vec3& getRedFlag() const   { return (Vec3&)m_red_trans.getOrigin(); }
    // ------------------------------------------------------------------------
    const Vec3& getBlueFlag() const { return (Vec3&)m_blue_trans.getOrigin(); }
    // ------------------------------------------------------------------------
    void loseFlagForKart(int kart_id);
    // ------------------------------------------------------------------------
    void resetKartForSwatterHit(int kart_id, int at_world_ticks)
                      { m_swatter_reset_kart_ticks[kart_id] = at_world_ticks; }
    // ------------------------------------------------------------------------
    virtual std::pair<uint32_t, uint32_t> getGameStartedProgress() const
        OVERRIDE
    {
        std::pair<uint32_t, uint32_t> progress(
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max());
        if (race_manager->hasTimeTarget())
        {
            progress.first = (uint32_t)m_time;
        }
        if (m_red_scores > m_blue_scores)
        {
            progress.second = (uint32_t)((float)m_red_scores /
                (float)race_manager->getHitCaptureLimit() * 100.0f);
        }
        else
        {
            progress.second = (uint32_t)((float)m_blue_scores /
                (float)race_manager->getHitCaptureLimit() * 100.0f);
        }
        return progress;
    }
};   // CaptureTheFlag

#endif
