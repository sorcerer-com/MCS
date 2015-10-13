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
            {"RenderElementType",  "ERenderElementType"},
            {"LightType",  "ELightType"},
            {"RendererType",  "ERendererType"},

            {"SceneElementPtr",  "MSceneElement^"},
            {"ContentElementPtr",  "MContentElement^"},
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
            else if (wrapperType == "MSceneElement^")
                converted = string.Format("this->getMSceneElement({0})", value);
            else if (wrapperType == "MContentElement^")
                converted = string.Format("this->getMContentElement({0})", value);

            return converted;
        }

        public static List<string> ConvertToWrapperListType(string value, string type, string wrapperType, string wrapperName, int indentCount = 0)
        {
            List<string> result = new List<string>();

            string innerMemeberType = type.Replace("vector<", "");
            innerMemeberType = innerMemeberType.Remove(innerMemeberType.Length - 1);
            string innerWrapperType = wrapperType.Replace("List<", "");
            innerWrapperType = innerWrapperType.Remove(innerWrapperType.Length - 2);
            string converted = WrapperTypesUtils.ConvertToWrapperType("value", innerMemeberType, innerWrapperType, wrapperName);

            result.Add("{0} collection = gcnew {1}();", wrapperType, wrapperType.Remove(wrapperType.Length - 1));
            result.Add("const auto& res = {0};", value);
            result.Add("for (const auto& value : res)");
            result.Add("{0}collection->Add({1});", CodeGenerator.Indent, converted);

            string indent = string.Empty;
            for (int i = 0; i < indentCount; i++)
                indent += CodeGenerator.Indent;
            for (int i = 0; i < result.Count; i++)
                result[i] = indent + result[i];

            return result;
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
