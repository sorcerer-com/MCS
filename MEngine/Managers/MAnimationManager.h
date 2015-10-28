// MAnimationManager.h
#pragma once

#include "Engine\Managers\AnimationManager.h"
#pragma managed

#include "MBaseManager.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"
#include "..\Utils\Types\MColor.h"


namespace MyEngine {

    public enum class EAnimTrackType
    {
        None,
        Float,
        MPoint,
        MColor
    };

    public ref class MAnimationManager : MBaseManager
	{
	private:
        AnimationManager* animationManager;

    public:
        using MAnimKeyFrame = array<double>;
        using MAnimationType = Dictionary < String^, Dictionary<int, MAnimKeyFrame^>^ >^;

		property List<String^>^ AnimationsNames
		{
            List<String^>^ get();
		}

#pragma region AnimationManager Properties_h
#pragma endregion


        delegate void ChangedEventHandler(MAnimationManager^ sender);
		event ChangedEventHandler^ Changed;

	public:
        MAnimationManager(MEngine^ owner, AnimationManager* animationManager);

#pragma region AnimationManager Functions_h
        bool AddAnimation(String^ name);
        bool ContainsAnimation(String^ name);
        bool RenameAnimation(String^ oldName, String^ newName);
        bool DeleteAnimation(String^ name);
        
        bool ContainsTrack(String^ animation, String^ track);
        bool DeleteTrack(String^ animation, String^ track);
        bool RemoveKeyframe(String^ animation, String^ track, int frame);
        
        void PlayAnimation(uint seID, String^ animation, double startTime, double startAt, bool paused, bool loop, double speed, bool linear);
        bool IsPlayingAnimation(uint seID);
        void StopAnimation(uint seID);
        double GetTime();
        void MoveTime(double deltaTime);
        void ResetTime();
#pragma endregion
        bool CloneAnimation(String^ name, String^ newName);
        MAnimationType GetAnimation(String^ name);
        bool AddTrack(String^ animation, String^ track, EAnimTrackType type);
        bool SetKeyframe(String^ animation, String^ track, int frame, MAnimKeyFrame^ mKeyframe);

	private:
		void OnChanged(String^ element);
    };

}
