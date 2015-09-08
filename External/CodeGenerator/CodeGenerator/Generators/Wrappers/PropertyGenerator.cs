using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System;
using System.Collections.Generic;

namespace CodeGenerator.Generators.Wrappers
{
    [Obsolete("Isn't ready yet", true)]
    public class PropertyGenerator : BaseGenerator
    {
        public static readonly Dictionary<string, string> TypesMap = new Dictionary<string, string>
        {
            {"long long",  "long"},
            {"string",  "String^"},
            {"Vector3", "MPoint"},
            {"Color4",  "MColor"},
        };


        public PropertyGenerator(CodeGenerator owner)
            : base(owner)
        {
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
                    string type = member.Type;
                    if (TypesMap.ContainsKey(type))
                        type = TypesMap[type];

                    result.Add("property {0} {1}", type, member.Name);
                    result.Add("{");
                    result.Add("\t{0} get() {{ return element->{1}; }}", type, member.Name);
                    result.Add("}");
                    result.Add("");
                }
            }

            return result;
        }
    }
}
