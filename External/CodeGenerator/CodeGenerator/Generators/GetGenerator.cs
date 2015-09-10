using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System.Collections.Generic;

namespace CodeGenerator.Generators
{
    public class GetGenerator : BaseGenerator
    {
        public GetGenerator(CodeGenerator owner)
            : base(owner)
        {
        }


        public override List<string> Generate(string _class)
        {
            List<string> result = new List<string>();

            List<Member> members = this.CodeGenerator.Members;
            string elseString = string.Empty;
            foreach (var member in members)
            {
                if (member.Access == "public" && member.Class == _class && !member.Pointer &&
                    !member.ContainsAttribute(MemberAttributes.NoGet))
                {
                    result.Add("{0}if (name == \"{1}\")", elseString, member.Name);
                    result.Add("{0}return &this->{1};", CodeGenerator.Indent, member.Name);
                    elseString = "else ";
                }
            }

            return result;
        }
    }
}
