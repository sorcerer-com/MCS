using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace CodeGenerator.Infos
{
    public static class MemberAttributes
    {
        public const string NoSave = "nosave";
        public const string NoInit = "noinit";
        public const string NoProperty = "noproperty";

        public const string ReadOnly = "readonly";
        public const string Default = "default";
    }

    public class Member
    {
        public string Class { get; set; }
        public string Access { get; set; }
        public string Type { get; set; }
        public string Name { get; set; }
        public Dictionary<string, string> Attributes { get; private set; }

        public bool Pointer { get { return this.Type.EndsWith("*"); } }

        public static Regex RegEx = new Regex(@"^\s*(?<type>[\w\d_<>\s]+\**)\s(?<name>[\w\d_]+);", RegexOptions.IgnoreCase); // [\w\d_<>\s] - type (all letter, digits, <, > and whitespace), \* - Pointer, 
        public static Regex AttributesRegEx = new Regex(@"//\*(\s*(?<attribute>([\w\d_]+)(\[[^\]^\[]*\])*)\s*)+(//)*", RegexOptions.IgnoreCase); // [\w\d_] - attribute name, \[[^\]^\[]*\] - all characters except [ and ] between []


        private Member()
        {
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
            return string.Format("{0} {1} {2}.{3}", this.Access, this.Type, this.Class, this.Name);
        }


        public static Member Read(string _class, string _access, string line)
        {
            if (string.IsNullOrEmpty(_class) || string.IsNullOrEmpty(_access))
                return null;

            Match match = RegEx.Match(line);
            if (!match.Success)
                return null;

            if (string.IsNullOrEmpty(match.Groups["type"].Value) ||
                string.IsNullOrEmpty(match.Groups["name"].Value))
            {
                Program.Log("Invalid member: " + line);
                return null;
            }

            Member member = new Member();
            member.Class = _class;
            member.Access = _access;
            member.Type = match.Groups["type"].Value.Trim();
            member.Name = match.Groups["name"].Value.Trim();

            // add attributes
            Match attMatch = AttributesRegEx.Match(line);
            if (attMatch.Success)
            {
                foreach (Capture capture in attMatch.Groups["attribute"].Captures)
                {
                    string[] att = capture.Value.Split(new char[] { '[', ']' }, StringSplitOptions.RemoveEmptyEntries);
                    member.Attributes.Add(att[0].Trim().ToLower(), att.Length > 1 ? att[1].Trim() : string.Empty);
                }
            }

            return member;
        }
    }
}
