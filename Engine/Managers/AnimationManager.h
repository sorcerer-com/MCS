// AnimationManager.h
#pragma once

#include "BaseManager.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {
#pragma warning (disable:4503)

    struct AnimKeyFrame
    {
        enum {
            ENone,
            EFloat,
            EVector3,
            EColor4
        } Type;

        float Value[4];

        AnimKeyFrame() = default;
        AnimKeyFrame(const float& value);
        AnimKeyFrame(const Vector3& value);
        AnimKeyFrame(const Color4& value);
        AnimKeyFrame(istream& file);

        void WriteToFile(ostream& file) const;
    };

	class AnimationManager : public BaseManager
	{
    public:
        using AnimTrack = map < int, AnimKeyFrame > ; // frame / keyframe
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
        AnimationType GetAnimation(const string& name);
        vector<string> GetAnimationsNames();

        bool AddTrack(const string& animation, const string& track);     //* wrap
        bool ContainsTrack(const string& animation, const string& track) const;  //* wrap
        bool DeleteTrack(const string& animation, const string& track);  //* wrap
        bool SetKeyframe(const string& animation, const string& track, uint frame, const AnimKeyFrame& keyframe);   //* wrap
        bool RemoveKeyframe(const string& animation, const string& track, uint frame);//* wrap
    };

}