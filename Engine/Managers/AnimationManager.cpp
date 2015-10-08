// AnimationManager.cpp

#include "stdafx.h"
#include "AnimationManager.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\IOUtils.h"
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Types\Vector3.h"


namespace MyEngine {

    // A n i m   K e y   F r a m e
    AnimTrack::AnimTrack(TrackType type)
    {
        this->Type = type;
    }

    AnimTrack::AnimTrack(istream& file)
    {
        Read(file, this->Type); // track's type

        // track's keyframes
        int keyframesCount = 0;
        Read(file, keyframesCount);
        for (int j = 0; j < keyframesCount; j++)
        {
            int frame = 0;
            Read(file, frame); // keyframe's frame

            // keyframe's value
            Read(file, this->KeyFrames[frame][0]);
            if (this->Type != TrackType::EFloat)
            {
                Read(file, this->KeyFrames[frame][1]);
                Read(file, this->KeyFrames[frame][2]);
                if (this->Type == TrackType::EColor4)
                    Read(file, this->KeyFrames[frame][3]);
            }
        }
    }

    bool AnimTrack::SetKeyframe(int frame, const float& value)
    {
        if (this->Type != TrackType::EFloat)
            return false;

        this->KeyFrames[frame][0] = value;
        return true;
    }

    bool AnimTrack::SetKeyframe(int frame, const Vector3& value)
    {
        if (this->Type != TrackType::EVector3)
            return false;

        this->KeyFrames[frame][0] = value.x;
        this->KeyFrames[frame][1] = value.y;
        this->KeyFrames[frame][2] = value.z;
        return true;
    }

    bool AnimTrack::SetKeyframe(int frame, const Color4& value)
    {
        if (this->Type != TrackType::EColor4)
            return false;

        this->KeyFrames[frame][0] = value.r;
        this->KeyFrames[frame][1] = value.g;
        this->KeyFrames[frame][2] = value.b;
        this->KeyFrames[frame][3] = value.a;
        return true;
    }

    void AnimTrack::WriteToFile(ostream& file) const
    {
        Write(file, this->Type); // track's type

        Write(file, (int)this->KeyFrames.size()); // keyframes count
        for (const auto& animKeyframe : this->KeyFrames)
        {
            Write(file, animKeyframe.first); // keframe's frame

            // keyframe's value
            Write(file, animKeyframe.second[0]);
            if (this->Type != TrackType::EFloat)
            {
                Write(file, animKeyframe.second[1]);
                Write(file, animKeyframe.second[2]);
                if (this->Type == TrackType::EColor4)
                    Write(file, animKeyframe.second[3]);
            }
        }
    }


	/* A N I M A T I O N   M A N A G E R */
    AnimationManager::AnimationManager(Engine* owner) :
        BaseManager(owner)
    {
        this->thread->defMutex("content");
    }


    void AnimationManager::ReadFromFile(istream& file)
    {
        this->animations.clear();

        // animations count
        int animCount = 0;
        Read(file, animCount);
        for (int i = 0; i < animCount; i++)
        {
            // animation's name
            string animName = "";
            Read(file, animName);
            this->animations[animName];

            // animation's tracks count
            int tracksCount = 0;
            Read(file, tracksCount);
            for (int j = 0; j < tracksCount; j++)
            {
                // track's name
                string trackName = "";
                Read(file, trackName);
                // track
                this->animations[animName][trackName] = AnimTrack(file);
            }
        }
    }

    void AnimationManager::WriteToFile(ostream& file) const
    {
        // animations count
        Write(file, (int)this->animations.size());

        // animations
        for (const auto& anim : this->animations)
        {
            // animation's name
            Write(file, anim.first);

            // animation's tracks count
            Write(file, (int)anim.second.size());

            for (const auto& animTrack : anim.second)
            {
                // track's name
                Write(file, animTrack.first);

                // track
                animTrack.second.WriteToFile(file);
            }
        }
    }


    /* A N I M A T I O N S */
    bool AnimationManager::AddAnimation(const string& name)
    {
        if (this->ContainsAnimation(name))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to add animation '" + name + "' that already exists");
            return false;
        }

        lock lck(this->thread->mutex("content"));
        this->animations[name];

        Engine::Log(ELog, "AnimationManager", "Add animation '" + name + "'");
        return true;
    }

    bool AnimationManager::ContainsAnimation(const string& name) const
    {
        return this->animations.find(name) != this->animations.end();
    }

    bool AnimationManager::RenameAnimation(const string& oldName, const string& newName)
    {
        if (!this->ContainsAnimation(oldName))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to rename non existent animation '" + oldName + "'");
            return false;
        }
        if (this->ContainsAnimation(newName))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to rename animation '" + oldName + "' to '" + newName +
                "', but there is already animation with this name");
            return false;
        }

        lock lck(this->thread->mutex("content"));
        AnimationType animation = this->animations[oldName];
        this->animations[oldName].clear();
        this->animations.erase(oldName);

        this->animations[newName] = animation;
        Engine::Log(ELog, "AnimationManager", "Rename animation '" + oldName + "' to '" + newName + "'");
        return true;
    }

    bool AnimationManager::DeleteAnimation(const string& name)
    {
        if (!this->ContainsAnimation(name))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to delete non existent animation '" + name + "'");
            return false;
        }

        lock lck(this->thread->mutex("content"));
        this->animations.erase(name);

        Engine::Log(ELog, "AnimationManager", "Delete animation '" + name + "'");
        return true;
    }
    
    vector<string> AnimationManager::GetAnimationsNames()
    {
        vector<string> result;

        for (const auto& anim : this->animations)
            result.push_back(anim.first);

        return result;
    }
    

    /* T R A C K S   A N D   K E Y F R A M E S */
    bool AnimationManager::AddTrack(const string& animation, const string& track, AnimTrack::TrackType type)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to add track '" + track + "' to non existent animation '" + animation + "'");
            return false;
        }
        if (this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to add track '" + track + "' that already exists");
            return false;
        }
        if (type == AnimTrack::TrackType::ENone)
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to add invalid type track");
            return false;
        }

        lock lck(this->thread->mutex("content"));
        this->animations[animation][track].Type = type;

        Engine::Log(ELog, "AnimationManager", "Add track '" + track + "' to animation '" + animation + "'");
        return true;
    }

    bool AnimationManager::ContainsTrack(const string& animation, const string& track) const
    {
        if (!this->ContainsAnimation(animation))
            return false;

        return this->animations.at(animation).find(track) != this->animations.at(animation).end();
    }

    bool AnimationManager::DeleteTrack(const string& animation, const string& track)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to delete track '" + track + "' from non existent animation '" + animation + "'");
            return false;
        }
        if (!this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to delete non existent track '" + track + "'");
            return false;
        }

        lock lck(this->thread->mutex("content"));
        this->animations[animation].erase(track);

        Engine::Log(ELog, "AnimationManager", "Delete track '" + track + "' from animation'" + animation + "'");
        return true;
    }

    bool AnimationManager::SetKeyframe(const string& animation, const string& track, uint frame, const float* keyframe)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to set key to non existent animation '" + animation + "'");
            return false;
        }
        if (!this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to set key to non existent track '" + track + "' in animation '" + animation + "'");
            return false;
        }

        for (int i = 0; i < 4; i++)
            this->animations[animation][track].KeyFrames[frame][i] = keyframe[i];

        return true;
    }

    bool AnimationManager::RemoveKeyframe(const string& animation, const string& track, int frame)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to remove keyframe from non existent animation '" + animation + "'");
            return false;
        }
        if (!this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to remove keyframe from non existent track '" + track + "' in animation '" + animation + "'");
            return false;
        }

        this->animations[animation][track].KeyFrames.erase(frame);

        return true;
    }

    AnimTrack AnimationManager::GetTrack(const string& animation, const string& track)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to get track from non existent animation '" + animation + "'");
            return AnimTrack();
        }
        if (!this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to get non existent track '" + track + "' in animation '" + animation + "'");
            return AnimTrack();
        }

        return this->animations[animation][track];
    }

    vector<string> AnimationManager::GetTracksNames(const string& animation)
    {
        vector<string> result;

        for (const auto& track : this->animations[animation])
            result.push_back(track.first);

        return result;
    }

}
