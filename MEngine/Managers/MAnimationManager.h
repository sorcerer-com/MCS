// MAnimationManager.h
#pragma once

#include "Engine\Managers\AnimationManager.h"
#pragma managed

#include "MBaseManager.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"
#include "..\Utils\Types\MColor.h"


namespace MyEngine {

    public enum class EAnimKeyFrameType
    {
        None,
        Float,
        MPoint,
        MColor
    };

    public value struct MAnimKeyFrame
    {
        EAnimKeyFrameType Type;
        property float Float;
        property MPoint Point;
        property MColor Color;
    };

    public ref class MAnimationManager : MBaseManager
	{
	private:
        AnimationManager* animationManager;

    public:
        using MAnimationType = Dictionary < String^, Dictionary<int, MAnimKeyFrame>^ >^;

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
        
        bool AddTrack(String^ animation, String^ track);
        bool ContainsTrack(String^ animation, String^ track);
        bool DeleteTrack(String^ animation, String^ track);
        bool SetKeyframe(String^ animation, String^ track, uint frame, AnimKeyFrame keyframe);
        bool RemoveKeyframe(String^ animation, String^ track, uint frame);
#pragma endregion
        bool CloneAnimation(String^ name, String^ newName);
        MAnimationType GetAnimation(String^ name);

	private:
		void OnChanged(String^ element);
    };

}
