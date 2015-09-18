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
    AnimKeyFrame::AnimKeyFrame(const float& value)
    {
        this->Type = AnimKeyFrame::EFloat;
        this->Value[0] = value;
    }

    AnimKeyFrame::AnimKeyFrame(const Vector3& value)
    {
        this->Type = AnimKeyFrame::EVector3;
        this->Value[0] = value.x;
        this->Value[1] = value.y;
        this->Value[2] = value.z;
    }

    AnimKeyFrame::AnimKeyFrame(const Color4& value)
    {
        this->Type = AnimKeyFrame::EColor4;
        this->Value[0] = value.r;
        this->Value[1] = value.g;
        this->Value[2] = value.b;
        this->Value[3] = value.a;
    }

    AnimKeyFrame::AnimKeyFrame(istream& file)
    {
        Read(file, this->Type);
        Read(file, this->Value[0]);
        if (this->Type != AnimKeyFrame::EFloat)
        {
            Read(file, this->Value[1]);
            Read(file, this->Value[2]);
            if (this->Type == AnimKeyFrame::EColor4)
                Read(file, this->Value[3]);
        }
    }

    void AnimKeyFrame::WriteToFile(ostream& file) const
    {
        Write(file, this->Type);
        Write(file, this->Value[0]);
        if (this->Type != AnimKeyFrame::EFloat)
        {
            Write(file, this->Value[1]);
            Write(file, this->Value[2]);
            if (this->Type == AnimKeyFrame::EColor4)
                Write(file, this->Value[3]);
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
                this->animations[animName][trackName];

                // track's keyframes
                int keyframesCount = 0;
                Read(file, keyframesCount);
                for (int j = 0; j < keyframesCount; j++)
                {
                    int frame = 0;
                    Read(file, frame); // keyframe's frame

                    this->animations[animName][trackName][frame] = AnimKeyFrame(file);
                }
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

                // track's keyframes count
                Write(file, (int)animTrack.second.size());

                for (const auto& animKeyframe : animTrack.second)
                {
                    Write(file, animKeyframe.first); // keframe's frame
                    animKeyframe.second.WriteToFile(file);
                }
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
        if (!this->DeleteAnimation(oldName))
            return false;

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

    AnimationManager::AnimationType AnimationManager::GetAnimation(const string& name)
    {
        if (!this->ContainsAnimation(name))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to get non existent animation '" + name + "'");
            return AnimationType();
        }

        return this->animations[name];
    }

    vector<string> AnimationManager::GetAnimationsNames()
    {
        vector<string> result;

        for (const auto& anim : this->animations)
            result.push_back(anim.first);

        return result;
    }
    

    /* T R A C K S   A N D   K E Y F R A M E S */
    bool AnimationManager::AddTrack(const string& animation, const string& track)
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

        lock lck(this->thread->mutex("content"));
        this->animations[animation][track];

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

    bool AnimationManager::SetKeyframe(const string& animation, const string& track, uint frame, const AnimKeyFrame& keyframe)
    {
        if (!this->ContainsAnimation(animation))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to set key to non existent animation '" + animation + "'");
            return false;
        }
        if (!this->ContainsTrack(animation, track))
        {
            Engine::Log(LogType::EWarning, "AnimationManager", "Try to set key to non existent track '" + track+ "' in animation '" + animation + "'");
            return false;
        }
        
        this->animations[animation][track][frame] = keyframe;

        return true;
    }

    bool AnimationManager::RemoveKeyframe(const string& animation, const string& track, uint frame)
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

        this->animations[animation][track].erase(frame);

        return true;
    }

}
