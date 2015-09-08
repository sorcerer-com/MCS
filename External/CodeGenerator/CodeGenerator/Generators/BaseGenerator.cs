using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace CodeGenerator.Generators
{
    public abstract class BaseGenerator
    {
        public CodeGenerator CodeGenerator { get; set; }


        public BaseGenerator(CodeGenerator owner)
        {
            this.CodeGenerator = owner;
        }

        abstract public List<string> Generate(string _class);
    }
}
