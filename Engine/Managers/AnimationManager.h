// AnimationManager.h
#pragma once

#include "BaseManager.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

    struct AnimTrack
    {
        enum TrackType {
            ENone,
            EFloat,
            EVector3,
            EColor4
        } Type;

        map<int, float[4]> KeyFrames;

        AnimTrack() = default;
        AnimTrack(TrackType type);
        AnimTrack(istream& file);

        bool SetKeyframe(int frame, const float& value);
        bool SetKeyframe(int frame, const Vector3& value);
        bool SetKeyframe(int frame, const Color4& value);
        void WriteToFile(ostream& file) const;
    };

    struct AnimStatus
    {
        float StartTime;
        string Animation;
        float CurrentTime;
        bool Paused;
        bool Loop;
        float Speed;
    };

	class AnimationManager : public BaseManager
	{
    public:
        using AnimationType = map < string, AnimTrack > ; // track name / track
        using AnimationsMapType = map < string, AnimationType > ; // name / animation

        using AnimationsStatusMapType = map < uint, AnimStatus > ; // scene element id / animation info

    private:
        AnimationsMapType animations;

        float time; // time from start in seconds
        AnimationsStatusMapType animationsStatuses;
        
	public:
        AnimationManager(Engine* owner);
        ~AnimationManager();

        void ReadFromFile(istream& file);
        void WriteToFile(ostream& file) const;

        bool AddAnimation(const string& name);              //* wrap
        bool ContainsAnimation(const string& name) const;   //* wrap
        bool RenameAnimation(const string& oldName, const string& newName); //* wrap
        bool DeleteAnimation(const string& name);           //* wrap endgroup
        vector<string> GetAnimationsNames();

        bool AddTrack(const string& animation, const string& track, AnimTrack::TrackType type);
        bool ContainsTrack(const string& animation, const string& track) const;  //* wrap
        bool DeleteTrack(const string& animation, const string& track);  //* wrap
        bool SetKeyframe(const string& animation, const string& track, uint frame, const float* keyframe);   //* wrap
        bool RemoveKeyframe(const string& animation, const string& track, int frame);//* wrap endgroup
        AnimTrack GetTrack(const string& animation, const string& track);
        vector<string> GetTracksNames(const string& animation);

        void PlayAnimation(uint seID, const string& animation, float startTime, float startAt, bool paused, bool loop, float speed);    //* wrap
        bool IsPlayingAnimation(uint seID) const;           //* wrap
        void StopAnimation(uint seID);                      //* wrap
        AnimStatus GetAnimationStatus(uint seID);           
        void MoveTime(float deltaTime);                     //* wrap

    private:
        void doAnimation();
        void applyAnimation(uint seID, const AnimStatus& animStatus, float deltaTime);
        bool getValue(const AnimTrack& animTrack, float time, float* out);
        float getAnimationLength(const string& name);

    };

}