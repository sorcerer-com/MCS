using System.Collections.Generic;

namespace CodeGenerator.Utils
{
    public static class WrapperTypesUtils
    {
        private static readonly Dictionary<string, string> TypesMap = new Dictionary<string, string>
        {
            {"long long",  "long"},
            {"float",  "double"},
            {"string",  "String^"},
            {"Vector3", "MPoint"},
            {"Quaternion", "MPoint"},
            {"Color4",  "MColor"},
            {"ContentElementType",  "EContentElementType"},
            {"SceneElementType",  "ESceneElementType"},
            {"LightType",  "ELightType"},
            {"RendererType",  "ERendererType"},
            {"SceneElementPtr",  "MSceneElement^"},
        };


        public static string GetWrapperType(string type)
        {
            if (WrapperTypesUtils.TypesMap.ContainsKey(type))
                return WrapperTypesUtils.TypesMap[type];

            if (type.StartsWith("vector<") && type.EndsWith(">"))
            {
                type = type.Replace("vector<", "");
                type = type.Remove(type.Length - 1);
                type = WrapperTypesUtils.GetWrapperType(type);
                type = "List<" + type + ">^";
            }

            return type;
        }

        public static string ConvertToWrapperType(string value, string type, string wrapperType, string wrapperName)
        {
            string converted = value;

            if (wrapperType == "double")
                converted = string.Format("{0}", converted);
            else if (wrapperType == "String^")
                converted = string.Format("gcnew String({0}.c_str())", converted);
            else if (wrapperType == "MPoint" && type == "Quaternion")
                converted = string.Format("MPoint({0}.toEulerAngle())", converted);
            else if (wrapperType == "MPoint")
                converted = string.Format("MPoint({0})", converted);
            else if (wrapperType == "MColor")
                converted = string.Format("MColor({0})", converted);
            else if (wrapperType.StartsWith("E") && wrapperType.EndsWith("Type")) // enum
                converted = string.Format("({0}){1}", wrapperType, converted);
            else if (wrapperType == "MContentElement^" && wrapperName != null)
            {
                int index = value.LastIndexOf("->") + 2;
                if (index == -1)
                    index = value.LastIndexOf(".") + 1;
                converted = string.Format("MContentManager::GetMContentElement({0}Get{1}())", value.Substring(0, index), wrapperName);
            }

            return converted;
        }

        public static string ConvertFromWrapperType(string value, string type, string wrapperType)
        {
            string converted = value;

            if (wrapperType == "double")
                converted = string.Format("(float){0}", converted);
            else if (wrapperType == "String^")
                converted = string.Format("to_string({0})", converted);
            else if (wrapperType == "MPoint" && type == "Quaternion")
                converted = string.Format("Quaternion({0}.ToVector3())", converted);
            else if (wrapperType == "MPoint")
                converted = string.Format("{0}.ToVector3()", converted);
            else if (wrapperType == "MColor")
                converted = string.Format("{0}.ToColor4()", converted);
            else if (wrapperType.StartsWith("E") && wrapperType.EndsWith("Type")) // enum
                converted = string.Format("({0}){1}", type, converted);
            else if (wrapperType == "MContentElement^")
                converted = string.Format("({0} != nullptr ? {0}->ID : 0)", converted);

            return converted;
        }
    }
}
