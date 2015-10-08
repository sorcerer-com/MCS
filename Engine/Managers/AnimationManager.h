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

	class AnimationManager : public BaseManager
	{
    public:
        using AnimationType = map < string, AnimTrack >; // track name / track
        using AnimationsMapType = map < string, AnimationType > ; // name / animation

    private:
        AnimationsMapType animations;
        
	public:
        AnimationManager(Engine* owner);

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
        bool RemoveKeyframe(const string& animation, const string& track, int frame);//* wrap
        AnimTrack GetTrack(const string& animation, const string& track);
        vector<string> GetTracksNames(const string& animation);
    };

}