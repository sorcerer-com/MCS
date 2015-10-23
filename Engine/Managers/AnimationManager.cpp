// AnimationManager.cpp

#include "stdafx.h"
#include "AnimationManager.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\IOUtils.h"
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"
#include "..\Managers\SceneManager.h"
#include "..\Managers\ContentManager.h"
#include "..\Scene Elements\SceneElement.h"


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
        this->time = 0;

        this->thread->defMutex("content");
        this->thread->defMutex("status");
        this->thread->defWorker(&AnimationManager::doAnimation, this);
    }

    AnimationManager::~AnimationManager()
    {
        this->MoveTime(-this->time);
    }


    void AnimationManager::ReadFromFile(istream& file)
    {
        this->animations.clear();
        this->animationStatuses.clear();

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

        // animations statuses count
        int animStatusesCount = 0;
        Read(file, animStatusesCount);
        for (int i = 0; i < animStatusesCount; i++)
        {
            uint seID = INVALID_ID;
            Read(file, seID);
            AnimStatus animStatus;
            Read(file, animStatus.StartTime);
            Read(file, animStatus.Animation);
            Read(file, animStatus.CurrentTime);
            Read(file, animStatus.Paused);
            Read(file, animStatus.Loop);
            Read(file, animStatus.Speed);
            this->animationStatuses[seID] = animStatus;
        }
    }

    void AnimationManager::WriteToFile(ostream& file) const
    {
        // animations count
        Write(file, (int)this->animations.size());
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

        // animations statuses count
        Write(file, (int)this->animationStatuses.size());
        // animations statuses
        for (const auto& animStatus : this->animationStatuses)
        {
            Write(file, animStatus.first);
            Write(file, animStatus.second.StartTime);
            Write(file, animStatus.second.Animation);
            Write(file, animStatus.second.CurrentTime);
            Write(file, animStatus.second.Paused);
            Write(file, animStatus.second.Loop);
            Write(file, animStatus.second.Speed);
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
    
    vector<string> AnimationManager::GetAnimationNames()
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

    vector<string> AnimationManager::GetTrackNames(const string& animation)
    {
        vector<string> result;

        for (const auto& track : this->animations[animation])
            result.push_back(track.first);

        return result;
    }


    /* A N I M A T I O N   S T A T U S */
    void AnimationManager::PlayAnimation(uint seID, const string& animation, float startTime, float startAt, bool paused, bool loop, float speed)
    {
        lock lck(this->thread->mutex("status"));
        if (!this->ContainsAnimation(animation))
            Engine::Log(EError, "AnimationManager", "Try to play non existent animation '" + animation + "'");
        else if (!this->Owner->SceneManager->ContainsElement(seID))
            Engine::Log(EError, "AnimationManager", "Try to play animation to non existent scene element (" + to_string(seID) + ")");
        else
        {
            AnimStatus animStatus;
            animStatus.StartTime = startTime;
            animStatus.Animation = animation;
            animStatus.CurrentTime = startAt;
            animStatus.Paused = paused;
            animStatus.Loop = loop;
            animStatus.Speed = speed;

            this->animationStatuses[seID] = animStatus;
            this->applyAnimation(seID, animStatus, startAt);
        }
    }

    bool AnimationManager::IsPlayingAnimation(uint seID) const
    {
        return this->animationStatuses.find(seID) != this->animationStatuses.end();
    }

    void AnimationManager::StopAnimation(uint seID)
    {
        lock lck(this->thread->mutex("status"));
        this->animationStatuses.erase(seID);
    }

    AnimStatus AnimationManager::GetAnimationStatus(uint seID)
    {
        if (!this->IsPlayingAnimation(seID))
            return AnimStatus();

        return this->animationStatuses[seID];
    }

    void AnimationManager::MoveTime(float deltaTime)
    {
        lock lck(this->thread->mutex("status"));

        if (this->time < -deltaTime && deltaTime < 0)
            deltaTime = -this->time;

        for (auto& animStatus : this->animationStatuses)
        {
            float dTime = deltaTime;
            float animLength = this->getAnimationLength(animStatus.second.Animation);
            while (animStatus.second.CurrentTime + dTime * animStatus.second.Speed < -animLength)
                dTime += animLength;

            if (!animStatus.second.Paused && this->time >= animStatus.second.StartTime)
                animStatus.second.CurrentTime += dTime * animStatus.second.Speed;
            if (animStatus.second.Loop && animStatus.second.CurrentTime > animLength)
                animStatus.second.CurrentTime = 0;

            if (this->time >= animStatus.second.StartTime)
                this->applyAnimation(animStatus.first, animStatus.second, dTime);
        }
        this->time += deltaTime;

        Engine::Log(LogType::EWarning, "AnimationManager", "Move time with " + to_string(deltaTime) + " to " + to_string(this->time));
    }


    void AnimationManager::doAnimation()
    {
        const float deltaTime = 1.0f / FPS;

        while (!this->thread->interrupted())
        {
            this->thread->mutex("status").lock();
            for (auto& animStatus : this->animationStatuses)
            {
                bool playing = this->Owner->Started && !animStatus.second.Paused && this->time >= animStatus.second.StartTime;
                if (playing)
                    animStatus.second.CurrentTime += deltaTime * animStatus.second.Speed;
                if (animStatus.second.Loop && animStatus.second.CurrentTime > this->getAnimationLength(animStatus.second.Animation))
                    animStatus.second.CurrentTime = 0;

                this->applyAnimation(animStatus.first, animStatus.second, playing ? deltaTime : 0.0f);
            }
            this->thread->mutex("status").unlock();

            if (!this->Owner->Started)
            {
                //if (this->time != 0)
                //    this->MoveTime(-this->time); // TODO: remove it if there will be Timeline
                this_thread::sleep_for(chrono::milliseconds((int)(deltaTime * 10000)));
            }
            else
            {
                this_thread::sleep_for(chrono::milliseconds((int)(deltaTime * 1000)));
                this->time += deltaTime;
            }
        }
    }

    void AnimationManager::applyAnimation(uint seID, const AnimStatus& animStatus, float deltaTime)
    {
        if (!this->ContainsAnimation(animStatus.Animation) ||
            !this->Owner->SceneManager->ContainsElement(seID) ||
            deltaTime == 0.0f)
            return;

        const auto& se = this->Owner->SceneManager->GetElement(seID);
        const auto& anim = this->animations[animStatus.Animation];
        for (const auto& track : anim)
        {
            float value[4], prev_value[4];
            if (!this->getValue(track.second, animStatus.CurrentTime, value))
                continue;

            this->getValue(track.second, animStatus.CurrentTime - deltaTime * animStatus.Speed, prev_value);
            if (track.second.Type == AnimTrack::TrackType::EFloat)
            {
                float f = value[0] - prev_value[0];
                if (track.first.find("Material") == 0)
                {
                    ContentElementPtr contentElement = NULL;
                    if (this->Owner->ContentManager->ContainsElement(se->MaterialID))
                    {
                        if (se->Type != SceneElementType::EDynamicObject)
                            contentElement = this->Owner->ContentManager->GetElement(se->MaterialID, true, true);
                        else
                            contentElement = this->Owner->ContentManager->GetInstance(se->ID, se->MaterialID);
                    }
                    if (contentElement && contentElement->Type == ContentElementType::EMaterial)
                    {
                        Material* mat = (Material*)contentElement.get();
                        string name = track.first.substr(9);
                        mat->Get<float>(name) += f;
                    }
                }
                else
                    se->Get<float>(track.first) += f;
            }
            else if(track.second.Type == AnimTrack::TrackType::EVector3)
            {
                Vector3 vec = Vector3(value[0] - prev_value[0], value[1] - prev_value[1], value[2] - prev_value[2]);
                if (track.first == "Rotation")
                    se->Get<Quaternion>(track.first) = Quaternion(vec) * se->Get<Quaternion>(track.first);
                else
                    se->Get<Vector3>(track.first) += vec;
            }
            else if (track.second.Type == AnimTrack::TrackType::EColor4)
            {
                Color4 col = Color4(value[0] - prev_value[0], value[1] - prev_value[1], value[2] - prev_value[2], value[3] - prev_value[3]);
                if (track.first.find("Material") == 0)
                {
                    ContentElementPtr contentElement = NULL;
                    if (this->Owner->ContentManager->ContainsElement(se->MaterialID))
                    {
                        if (se->Type != SceneElementType::EDynamicObject)
                            contentElement = this->Owner->ContentManager->GetElement(se->MaterialID, true, true);
                        else
                            contentElement = this->Owner->ContentManager->GetInstance(se->ID, se->MaterialID);
                    }
                    if (contentElement && contentElement->Type == ContentElementType::EMaterial)
                    {
                        Material* mat = (Material*)contentElement.get();
                        string name = track.first.substr(9);
                        mat->Get<Color4>(name) += col;
                    }
                }
                else
                    se->Get<Color4>(track.first) += col;
            }
        }
    }

    bool AnimationManager::getValue(const AnimTrack& animTrack, float time, float* out)
    {
        // if we are outside the animation
        if (animTrack.KeyFrames.size() == 0 ||
            time < (*animTrack.KeyFrames.begin()).first)
        {
            for (int i = 0; i < 4; i++)
                out[i] = 0;
            return false;
        }
        if (time > (*animTrack.KeyFrames.rbegin()).first)
        {
            for (int i = 0; i < 4; i++)
                out[i] = (*animTrack.KeyFrames.rbegin()).second[i];
            return false;
        }

        int frame = (int)(time * FPS);
        int prevKeyframeTime = -1;
        for(const auto& keyframe : animTrack.KeyFrames)
        {
            if (frame <= keyframe.first)
            {
                if (keyframe.first < 0.0)
                    return false;

                float d = (float)(keyframe.first - prevKeyframeTime);
                float t = (frame - prevKeyframeTime) / d;
                const auto& prevKeyrame = prevKeyframeTime >= 0 ? animTrack.KeyFrames.at(prevKeyframeTime) : keyframe.second;
                // cubic bezier
                for (int i = 0; i < 4; i++)
                    out[i] = pow(1.0f - t, 3) * prevKeyrame[i] + 3 * pow(1.0f - t, 2) * t * prevKeyrame[i] + 3 * (1.0f - t) * pow(t, 2) * keyframe.second[i] + pow(t, 3) * keyframe.second[i];
                return true;
            }
            prevKeyframeTime = keyframe.first;
        }
        return false;
    }

    float AnimationManager::getAnimationLength(const string& name)
    {
        int res = 0;

        const auto& anim = this->animations[name];
        for (const auto& track : anim)
            if (track.second.KeyFrames.size() > 0)
                res = max(res, (*track.second.KeyFrames.rbegin()).first);

        return (float)res / FPS;
    }

}
