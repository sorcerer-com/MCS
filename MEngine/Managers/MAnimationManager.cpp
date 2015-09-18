// MAnimationManager.cpp

#include "stdafx.h"
#include "MAnimationManager.h"



namespace MyEngine {

    List<String^>^ MAnimationManager::AnimationsNames::get()
    {
        List<String^>^ collection = gcnew List<String^>();

        const auto& animationsNames = this->animationManager->GetAnimationsNames();
        for (const auto& name : animationsNames)
            collection->Add(gcnew String(name.c_str()));

        return collection;
    }

#pragma region AnimationManager Properties_cpp
#pragma endregion


    MAnimationManager::MAnimationManager(MEngine^ owner, AnimationManager* animationManager) :
        MBaseManager(owner)
	{
        this->animationManager = animationManager;
	}


#pragma region AnimationManager Functions_cpp
	bool MAnimationManager::AddAnimation(String^ name)
	{
	    bool res = this->animationManager->AddAnimation(to_string(name));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MAnimationManager::ContainsAnimation(String^ name)
	{
	    return this->animationManager->ContainsAnimation(to_string(name));
	}
	
	bool MAnimationManager::RenameAnimation(String^ oldName, String^ newName)
	{
	    bool res = this->animationManager->RenameAnimation(to_string(oldName), to_string(newName));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MAnimationManager::DeleteAnimation(String^ name)
	{
	    bool res = this->animationManager->DeleteAnimation(to_string(name));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	
	bool MAnimationManager::AddTrack(String^ animation, String^ track)
	{
	    bool res = this->animationManager->AddTrack(to_string(animation), to_string(track));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MAnimationManager::ContainsTrack(String^ animation, String^ track)
	{
	    return this->animationManager->ContainsTrack(to_string(animation), to_string(track));
	}
	
	bool MAnimationManager::DeleteTrack(String^ animation, String^ track)
	{
	    bool res = this->animationManager->DeleteTrack(to_string(animation), to_string(track));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MAnimationManager::SetKeyframe(String^ animation, String^ track, uint frame, AnimKeyFrame keyframe)
	{
	    bool res = this->animationManager->SetKeyframe(to_string(animation), to_string(track), frame, keyframe);
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MAnimationManager::RemoveKeyframe(String^ animation, String^ track, uint frame)
	{
	    bool res = this->animationManager->RemoveKeyframe(to_string(animation), to_string(track), frame);
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
#pragma endregion

    bool MAnimationManager::CloneAnimation(String^ name, String^ newName)
    {
        if (!this->animationManager->ContainsAnimation(to_string(name)) ||
            !this->animationManager->AddAnimation(to_string(newName)))
            return false;

        string newAnimName = to_string(newName);
        const auto& animation = this->animationManager->GetAnimation(to_string(name));
        for (const auto& track : animation)
        {
            this->animationManager->AddTrack(newAnimName, track.first);
            for (const auto& keyframe : track.second)
                this->animationManager->SetKeyframe(newAnimName, track.first, keyframe.first, keyframe.second);
        }

        this->OnChanged(nullptr);
        return true;
    }

    MAnimationManager::MAnimationType MAnimationManager::GetAnimation(String^ name)
    {
        Dictionary<String^, Dictionary<int, MAnimKeyFrame>^>^ result = 
            gcnew Dictionary<String^, Dictionary<int, MAnimKeyFrame>^>();

        const auto& animation = this->animationManager->GetAnimation(to_string(name));
        for (const auto& track : animation)
        {
            String^ trackName = gcnew String(track.first.c_str());
            result->Add(trackName, gcnew Dictionary<int, MAnimKeyFrame>());

            for (const auto& keyframe : track.second)
            {
                MAnimKeyFrame mKeyframe;
                mKeyframe.Type = (EAnimKeyFrameType)keyframe.second.Type;
                if (mKeyframe.Type == EAnimKeyFrameType::Float)
                    mKeyframe.Float = keyframe.second.Value[0];
                else if (mKeyframe.Type == EAnimKeyFrameType::MPoint)
                    mKeyframe.Point = MPoint(keyframe.second.Value[0], keyframe.second.Value[1], keyframe.second.Value[2]);
                else if (mKeyframe.Type == EAnimKeyFrameType::MColor)
                    mKeyframe.Color = MColor(keyframe.second.Value[0], keyframe.second.Value[1], keyframe.second.Value[2], keyframe.second.Value[3]);
                result[trackName]->Add(keyframe.first, mKeyframe);
            }
        }

        return result;
    }


    void MAnimationManager::OnChanged(String^)
	{
		this->Changed(this);
	}

}
