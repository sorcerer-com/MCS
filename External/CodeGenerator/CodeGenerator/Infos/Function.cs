using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace CodeGenerator.Infos
{
    public static class FunctionAttributes
    {
        public const string Wrap = "wrap";
        public const string EndGroup = "endgroup";
    }

    public class Function
    {
        public string Class { get; set; }
        public string Access { get; set; }
        public bool Virtual { get; set; }
        public string ReturnType { get; set; }
        public string Name { get; set; }
        public Dictionary<string, string> Parameters { get; private set; } // name / type
        public bool Constant { get; set; }
        public Dictionary<string, string> Attributes { get; private set; }

        public static Regex RegEx = new Regex(@"^\s*(?<virtual>(virtual)*)(?<type>[\w\d_<>\s]+\**)\s(?<name>[\w\d_]+)\((?<param>\s*([\w\d_<>\s&]+)\s([\w\d_]+(\s*=\s*.*)*)(,*))*\)(?<const>(\sconst)*);", RegexOptions.IgnoreCase); // for params: [\w\d_<>\s] - type, [\w\d_] - name
        public static Regex AttributesRegEx = Member.AttributesRegEx;


        private Function()
        {
            this.Parameters = new Dictionary<string, string>();
            this.Attributes = new Dictionary<string, string>();
        }

        public bool ContainsAttribute(string attribute)
        {
            return this.Attributes.ContainsKey(attribute);
        }

        public string GetAttribute(string attribute)
        {
            if (!this.Attributes.ContainsKey(attribute))
                return null;

            return this.Attributes[attribute];
        }
        
        public override string ToString()
        {
            string args = string.Empty;
            foreach(var pair in this.Parameters)
                args += string.Format("{0} {1}, ", pair.Value, pair.Key);
            if (args.Length > 2)
                args = args.Remove(args.Length - 2);
            return string.Format("{0} {1} {2}.{3}({4})", this.Access, this.ReturnType, this.Class, this.Name, args);
        }


        public static Function Read(string _class, string _access, string line)
        {
            if (string.IsNullOrEmpty(_class) || string.IsNullOrEmpty(_access))
                return null;

            Match match = RegEx.Match(line);
            if (!match.Success)
                return null;

            if (string.IsNullOrEmpty(match.Groups["type"].Value) ||
                string.IsNullOrEmpty(match.Groups["name"].Value))
            {
                Program.Log("Invalid function: " + line);
                return null;
            }

            Function func = new Function();
            func.Class = _class;
            func.Access = _access;
            func.Virtual = !string.IsNullOrEmpty(match.Groups["virtual"].Value.Trim());
            func.ReturnType = match.Groups["type"].Value.Trim();
            func.Name = match.Groups["name"].Value.Trim();
            func.Constant = !string.IsNullOrEmpty(match.Groups["const"].Value.Trim());

            // parameters
            foreach (Capture capture in match.Groups["param"].Captures)
            {
                string param = capture.Value.Trim();
                if (param[param.Length - 1] == ',') param = param.Remove(param.Length - 1);

                int lastWhiteSpace = -1;
                for (int i = param.Length - 2; i >= 1; i-- )
                {
                    if (char.IsWhiteSpace(param[i]) && param[i + 1] != '=' && param[i - 1] != '=')
                    {
                        lastWhiteSpace = i;
                        break;
                    }
                }

                func.Parameters.Add(param.Substring(lastWhiteSpace).Trim(), param.Substring(0, lastWhiteSpace).Trim());
            }

            // add attributes
            Match attMatch = AttributesRegEx.Match(line);
            if (attMatch.Success)
            {
                foreach (Capture capture in attMatch.Groups["attribute"].Captures)
                {
                    string[] att = capture.Value.Split(new char[] { '[', ']' }, StringSplitOptions.RemoveEmptyEntries);
                    func.Attributes.Add(att[0].Trim().ToLower(), att.Length > 1 ? att[1].Trim() : string.Empty);
                }
            }

            return func;
        }
    }
}
