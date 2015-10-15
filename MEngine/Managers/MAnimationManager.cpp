// MAnimationManager.cpp

#include "stdafx.h"
#include "MAnimationManager.h"

#include "..\MEngine.h"


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
	
	bool MAnimationManager::RemoveKeyframe(String^ animation, String^ track, int frame)
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

        string oldAnimName = to_string(name);
        string newAnimName = to_string(newName);
        const auto& tracksNames = this->animationManager->GetTracksNames(oldAnimName);
        for (const auto& trackName : tracksNames)
        {
            const auto& oldTrack = this->animationManager->GetTrack(oldAnimName, trackName);

            this->animationManager->AddTrack(newAnimName, trackName, oldTrack.Type);
            for (const auto& keyframe : oldTrack.KeyFrames)
                this->animationManager->SetKeyframe(newAnimName, trackName, keyframe.first, keyframe.second);
        }

        this->OnChanged(nullptr);
        return true;
    }

    MAnimationManager::MAnimationType MAnimationManager::GetAnimation(String^ name)
    {
        Dictionary<String^, Dictionary<int, MAnimKeyFrame^>^>^ result =
            gcnew Dictionary<String^, Dictionary<int, MAnimKeyFrame^>^>();

        const auto& tracksNames = this->animationManager->GetTracksNames(to_string(name));
        for (const auto& trackName : tracksNames)
        {
            String^ trackNameStr = gcnew String(trackName.c_str());
            result->Add(trackNameStr, gcnew Dictionary<int, MAnimKeyFrame^>());

            const auto& track = this->animationManager->GetTrack(to_string(name), trackName);
            for (const auto& keyframe : track.KeyFrames)
            {
                MAnimKeyFrame^ mKeyframe = nullptr;
                if (track.Type == AnimTrack::TrackType::EFloat)
                    mKeyframe = gcnew MAnimKeyFrame(1);
                else if (track.Type == AnimTrack::TrackType::EVector3)
                    mKeyframe = gcnew MAnimKeyFrame(3);
                else if (track.Type == AnimTrack::TrackType::EColor4)
                    mKeyframe = gcnew MAnimKeyFrame(4);
                else
                    continue;

                for (int i = 0; i < mKeyframe->Length; i++)
                    mKeyframe[i] = keyframe.second[i];
                result[trackNameStr]->Add(keyframe.first, mKeyframe);
            }
            if (track.KeyFrames.size() == 0) // if track is empty then add dummy 0 keyframe
            {
                MAnimKeyFrame^ mKeyframe = nullptr;
                if (track.Type == AnimTrack::TrackType::EFloat)
                    mKeyframe = gcnew MAnimKeyFrame(1);
                else if (track.Type == AnimTrack::TrackType::EVector3)
                    mKeyframe = gcnew MAnimKeyFrame(3);
                else if (track.Type == AnimTrack::TrackType::EColor4)
                    mKeyframe = gcnew MAnimKeyFrame(4);
                if (mKeyframe != nullptr)
                    result[trackNameStr]->Add(0, mKeyframe);
            }
        }

        return result;
    }

    bool MAnimationManager::AddTrack(String^ animation, String^ track, EAnimTrackType type)
    {
        bool res = this->animationManager->AddTrack(to_string(animation), to_string(track), (AnimTrack::TrackType)type);
        if (res)
            this->OnChanged(nullptr);
        return res;
    }

    bool MAnimationManager::SetKeyframe(String^ animation, String^ track, int frame, MAnimKeyFrame^ mKeyframe)
    {
        float value[4];
        for (int i = 0; i < mKeyframe->Length; i++)
            value[i] = (float)mKeyframe[i];

        bool res = this->animationManager->SetKeyframe(to_string(animation), to_string(track), frame, value);
        if (res)
            this->OnChanged(nullptr);
        return res;
    }


    void MAnimationManager::OnChanged(String^)
	{
		this->Changed(this);
	}

}
