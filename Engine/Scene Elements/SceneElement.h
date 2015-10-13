// SceneElement.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"
#include "..\Content Elements\Material.h"

namespace MyEngine {

	class SceneManager;

	class ContentElement;
	using ContentElementPtr = shared_ptr < ContentElement >;

	enum SceneElementType
	{
		ECamera,
        ELight,
        ESkyBox,
		ERenderObject,
		EStaticObject,
        EDynamicObject
	};

	class SceneElement
	{
    public:
        SceneManager* Owner;                //* noinit

		uint Version;                       //* default[CURRENT_VERSION] nosave noget readonly
		SceneElementType Type;              //* default[SceneElementType::ECamera] group["Base"] readonly
		uint ID;                            //* default[INVALID_ID] group["Base"] readonly
		string Name;                        //* group["Base"] readonly
		string Layer;                       //* default[DEFAULT_LAYER_NAME] group["Base"] readonly
		uint ContentID;                     //* default[INVALID_ID] group["Content"] choosable[true]
		uint MaterialID;                    //* default[INVALID_ID] group["Content"] choosable[true]
        TextureSet Textures;                //* noinit noproperty
		// TODO: attach?
		bool Visible;                       //* default[true] group["Base"]
		Vector3 Position;                   //* group["Transform"]
		Quaternion Rotation;                //* group["Transform"]
		Vector3 Scale;                      //* default[Vector3(1.0, 1.0, 1.0)] group["Transform"]

	public:
		SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID);
		SceneElement(SceneManager* owner, istream& file);
        virtual ~SceneElement();

		ContentElementPtr GetContent() const;
        ContentElementPtr GetMaterial() const;
        ContentElementPtr GetDiffuseMap() const;
        ContentElementPtr GetNormalMap() const;

        template <typename T>
        T& Get(const string& name) { return *((T*)this->get(name)); }
		virtual void WriteToFile(ostream& file) const;
		virtual SceneElement* Clone() const;

    protected:
		void init();
        virtual void* get(const string& name);
	};

}

