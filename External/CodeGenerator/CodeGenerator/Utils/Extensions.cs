using System.Collections.Generic;

namespace CodeGenerator.Utils
{
    static class Extensions
    {
        public static void Add(this List<string> list, string format, params object[] args)
        {
            list.Add(string.Format(format, args));
        }
    }
}
