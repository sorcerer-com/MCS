using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System.Collections.Generic;

namespace CodeGenerator.Generators.Serialization
{
    public class SizeGenerator : BaseGenerator
    {
        public SizeGenerator(CodeGenerator owner)
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
                    !member.ContainsAttribute(MemberAttributes.NoSave))
                {
                    result.Add("size += SizeOf(this->{0});", member.Name);
                }
            }

            return result;
        }
    }
}
