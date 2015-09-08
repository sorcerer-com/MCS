using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System.Collections.Generic;

namespace CodeGenerator.Generators
{
    public class InitGenerator : BaseGenerator
    {
        public InitGenerator(CodeGenerator owner)
            : base(owner)
        {
        }


        public override List<string> Generate(string _class)
        {
            List<string> result = new List<string>();

            List<Member> members = this.CodeGenerator.Members;
            foreach (var member in members)
            {
                if (member.Class == _class &&
                    !member.ContainsAttribute(MemberAttributes.NoInit))
                {
                    string value = GetDefaultValue(member);
                    if (!string.IsNullOrEmpty(value))
                        result.Add("this->{0} = {1};", member.Name, value);
                    else
                        Program.Log("There isn't default value for this member: " + member);
                }
            }

            return result;
        }


        public static string GetDefaultValue(Member member)
        {
            string value = member.GetAttribute(MemberAttributes.Default);
            if (!string.IsNullOrEmpty(value))
                return value;

            if (member.Pointer)
                return "NULL";

            switch (member.Type)
            {
                case "bool":
                    value = "false";
                    break;

                case "int":
                case "uint":
                case "long long":
                    value = "0";
                    break;

                case "float":
                    value = "0.0f";
                    break;

                case "string":
                    value = "\"\"";
                    break;

                default:
                    value = member.Type + "()";
                    break;
            }

            return value;
        }
    }
}
