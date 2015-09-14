using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System;
using System.Collections.Generic;

namespace CodeGenerator.Generators.Wrappers
{
    public class PropertyGenerator : BaseGenerator
    {
        public enum PropertyGeneratorType
        {
            Header_Only,
            Header,
            Code
        }

        public PropertyGeneratorType Type { get; set; }


        public PropertyGenerator(CodeGenerator owner, PropertyGeneratorType type)
            : base(owner)
        {
            this.Type = type;
        }


        public override List<string> Generate(string _class)
        {
            List<string> result = new List<string>();

            List<Member> members = this.CodeGenerator.Members;
            foreach (var member in members)
            {
                if (member.Access == "public" && member.Class == _class && !member.Pointer &&
                    !member.ContainsAttribute(MemberAttributes.NoProperty))
                {
                    string wrapperType = WrapperTypesUtils.GetWrapperType(member.Type);
                    string wrapperName = member.Name;

                    if (wrapperType == "uint" && member.Name != "ID" && member.Name.EndsWith("ID"))
                    {
                        wrapperType = "MContentElement^";
                        wrapperName = wrapperName.Remove(wrapperName.Length - 2);
                    }

                    if (this.Type == PropertyGeneratorType.Header_Only)
                        result.AddRange(generateHeaderOnly(member, wrapperType, wrapperName));
                    else if (this.Type == PropertyGeneratorType.Header)
                        result.AddRange(generateHeader(member, wrapperType, wrapperName));
                    else if (this.Type == PropertyGeneratorType.Code)
                        result.AddRange(generateCode(member, wrapperType, wrapperName));
                }
            }
            if (result.Count > 0 && string.IsNullOrWhiteSpace(result[result.Count - 1]))
                result.RemoveAt(result.Count - 1); // remove last empty line
            
            return result;
        }


        private List<string> generateHeaderOnly(Member member, string wrapperType, string wrapperName)
        {
            List<string> result = new List<string>();

            string attribute = PropertyGenerator.GetAttribute(member);
            if (!string.IsNullOrEmpty(attribute))
                result.Add(attribute);

            result.Add("property {0} {1}", wrapperType, wrapperName);
            result.Add("{");
            // getter
            if (wrapperType.StartsWith("List<"))
            {
                result.Add("{0}{1} get()", CodeGenerator.Indent, wrapperType);
                result.Add("{0}{{", CodeGenerator.Indent);
                result.AddRange(WrapperTypesUtils.ConvertToWrapperListType(PropertyGenerator.GetAccessMember(member) + "->" + member.Name, member.Type, wrapperType, wrapperName, 2));
                result.Add("{0}{0}return collection;", CodeGenerator.Indent);
                result.Add("{0}}}", CodeGenerator.Indent);
            }
            else
            {
                string converted = WrapperTypesUtils.ConvertToWrapperType(PropertyGenerator.GetAccessMember(member) + "->" + member.Name, member.Type, wrapperType, wrapperName);
                result.Add("{0}{1} get() {{ return {2}; }}", CodeGenerator.Indent, wrapperType, converted);
            }
            // setter
            if (!member.ContainsAttribute(MemberAttributes.ReadOnly))
            {
                if (wrapperType.StartsWith("List<"))
                    Program.Log("There isn't setter for List types");
                else
                {
                    string converted = WrapperTypesUtils.ConvertFromWrapperType("value", member.Type, wrapperType);
                    result.Add("{0}void set({1} value) {{ {2}->{3} = {4}; OnChanged(); }}", CodeGenerator.Indent, wrapperType, PropertyGenerator.GetAccessMember(member), member.Name, converted);
                }
            }
            result.Add("}");
            result.Add("");

            for (int i = 0; i < result.Count; i++)
                result[i] = CodeGenerator.Indent + result[i];

            return result;
        }
        
        private List<string> generateHeader(Member member, string wrapperType, string wrapperName)
        {
            List<string> result = new List<string>();

            string attribute = PropertyGenerator.GetAttribute(member);
            if (!string.IsNullOrEmpty(attribute))
                result.Add(attribute);

            result.Add("property {0} {1}", wrapperType, wrapperName);
            result.Add("{");
            // getter
            result.Add("{0}{1} get();", CodeGenerator.Indent, wrapperType);
            // setter
            if (!member.ContainsAttribute(MemberAttributes.ReadOnly))
            {
                if (wrapperType.StartsWith("List<"))
                    Program.Log("There isn't setter for List types");
                else
                    result.Add("{0}void set({1} value);", CodeGenerator.Indent, wrapperType);
            }
            result.Add("}");
            result.Add("");

            return result;
        }

        private List<string> generateCode(Member member, string wrapperType, string wrapperName)
        {
            List<string> result = new List<string>();
            
            // getter
            if (wrapperType.StartsWith("List<"))
            {
                result.Add("{0} M{1}::{2}::get()", wrapperType, member.Class, wrapperName);
                result.Add("{");
                result.AddRange(WrapperTypesUtils.ConvertToWrapperListType(PropertyGenerator.GetAccessMember(member) + "->" + member.Name, member.Type, wrapperType, wrapperName, 1));
                result.Add("{0}return collection;", CodeGenerator.Indent);
                result.Add("}");
            }
            else
            {
                string converted = WrapperTypesUtils.ConvertToWrapperType(PropertyGenerator.GetAccessMember(member) + "->" + member.Name, member.Type, wrapperType, wrapperName);
                result.Add("{0} M{1}::{2}::get()", wrapperType, member.Class, wrapperName);
                result.Add("{");
                result.Add("{0} return {1};", CodeGenerator.Indent, converted);
                result.Add("}");
            }
            result.Add("");
            // setter
            if (!member.ContainsAttribute(MemberAttributes.ReadOnly))
            {

                if (wrapperType.StartsWith("List<"))
                    Program.Log("There isn't setter for List types");
                else
                {
                    string converted = WrapperTypesUtils.ConvertFromWrapperType("value", member.Type, wrapperType);
                    result.Add("void M{0}::{1}::set({2} value)", member.Class, wrapperName, wrapperType);
                    result.Add("{");
                    result.Add("{0}{1}->{2} = {3};", CodeGenerator.Indent, PropertyGenerator.GetAccessMember(member), member.Name, converted);
                    result.Add("{0}OnChanged(nullptr);", CodeGenerator.Indent);
                    result.Add("}");
                    result.Add("");
                }
            }

            return result;
        }


        public static string GetAttribute(Member member)
        {
            Dictionary<string, string> attributes = new Dictionary<string, string>
            {
                {"Group",  ""},
                {"Name",  ""},
                {"SortName",  ""},
                {"Description",  ""},
                {"Choosable",  ""},
            };

            bool empty = true;
            List<string> keys = new List<string>(attributes.Keys);
            foreach (var key in keys)
            {
                attributes[key] = member.GetAttribute(key.ToLower());
                if (empty && !string.IsNullOrEmpty(attributes[key]))
                    empty = false;
            }

            if (empty)
                return string.Empty;

            string result = "[MPropertyAttribute(";
            foreach (var pair in attributes)
            {
                if (!string.IsNullOrEmpty(pair.Value))
                    result += string.Format("{0} = {1}, ", pair.Key, pair.Value);
            }
            result = result.Remove(result.Length - 2);
            result += ")]";
            return result;
        }

        public static string GetAccessMember(Member member)
        {
            return "this->" + char.ToLower(member.Class[0]) + member.Class.Substring(1);
        }

    }
}
