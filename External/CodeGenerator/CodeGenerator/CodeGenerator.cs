using CodeGenerator.Generators;
using CodeGenerator.Generators.Serialization;
using CodeGenerator.Generators.Wrappers;
using CodeGenerator.Infos;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace CodeGenerator
{
    public enum EResult
    {
        NoFile,
        Success,
        NoChanges
    }

    public class CodeGenerator
    {
        public string HeaderFileName { get; set; }
        public string CodeFileName { get; set; }
        public string WrapperHeaderFileName { get; set; }

        public List<Member> Members { get; private set; }

        private Dictionary<string, BaseGenerator> generators;


        public CodeGenerator(string headerFileName)
        {
            this.HeaderFileName = headerFileName;
            string dir = Path.GetDirectoryName(headerFileName);
            string fileName = Path.GetFileNameWithoutExtension(headerFileName);
            this.CodeFileName = Path.Combine(dir, fileName) + ".cpp";
            this.WrapperHeaderFileName = Path.Combine(dir.Replace(@"\Engine\", @"\MEngine\"), "M" + fileName) + ".h";

            this.Members = new List<Member>();

            this.generators = new Dictionary<string, BaseGenerator>();
            // Serialization
            this.generators.Add("Read", new ReadGenerator(this));
            this.generators.Add("Write", new WriteGenerator(this));
            this.generators.Add("Size", new SizeGenerator(this));
            // Initialization
            this.generators.Add("Init", new InitGenerator(this));
            // Wrappers
            // TODO: this.generators.Add("Property", new PropertyGenerator(this));
        }


        public List<EResult> Proccess()
        {
            List<EResult> result = new List<EResult>();
            
            result.Add(this.analyzeHeader());

            if (result[0] == EResult.Success)
            {
                result.Add(this.generateCode(this.CodeFileName));
                // TODO: result.Add(this.generateCode(this.WrapperHeaderFileName));
            }

            return result;
        }


        private EResult analyzeHeader()
        {
            if (!File.Exists(this.HeaderFileName))
                return EResult.NoFile;

            string[] lines = File.ReadAllLines(this.HeaderFileName);

            string _class = null;
            string _access = null;
            Regex classRegex = new Regex(@"^\s*class\s+(?<className>[\w\d_]+)", RegexOptions.IgnoreCase);
            Regex accessRegex = new Regex(@"^\s*(?<accessName>public|private|protected):", RegexOptions.IgnoreCase);
            
            for (int i = 0; i < lines.Length; i++)
            {
                string line = lines[i];

                Match classMatch = classRegex.Match(line);
                if (classMatch.Success)
                {
                    _class = classMatch.Groups["className"].Value;
                    continue;
                }

                Match accessMatch = accessRegex.Match(line);
                if (accessMatch.Success)
                {
                    _access = accessMatch.Groups["accessName"].Value;
                    continue;
                }

                Member member = Member.Read(_class, _access, line);
                if (member != null)
                {
                    this.Members.Add(member);
                    continue;
                }
            }
            return EResult.Success;
        }

        private EResult generateCode(string file)
        {
            if (!File.Exists(file))
                return EResult.NoFile;

            EResult result = EResult.NoChanges;
            List<string> lines = File.ReadAllLines(file).ToList();

            Regex beginRegionRegex = new Regex(@"^\s*#pragma region\s+(?<className>[\w\d_]+)\s+(?<type>[\w\d_]+)", RegexOptions.IgnoreCase);
            Regex endRegionRegex = new Regex(@"^\s*#pragma endregion", RegexOptions.IgnoreCase);
            for (int i = 0; i < lines.Count; i++)
            {
                string line = lines[i];

                Match beginRegionMatch = beginRegionRegex.Match(line);
                if (beginRegionMatch.Success)
                {
                    // count previous depth
                    string indent = string.Empty;
                    for (int k = 1; k <= i; k++)
                    {
                        if (string.IsNullOrEmpty(lines[i - k]))
                            continue;

                        for (int j = 0; j < lines[i - k].Length; j++)
                        {
                            if (!char.IsWhiteSpace(lines[i - k][j]))
                                break;
                            indent += lines[i - k][j];
                        }
                        if (lines[i - k].EndsWith("{"))
                            indent += "\t";
                        break;
                    }

                    // generate
                    string type = beginRegionMatch.Groups["type"].Value;
                    if (this.generators.ContainsKey(type))
                    {
                        BaseGenerator generator = this.generators[type];
                        var code = generator.Generate(beginRegionMatch.Groups["className"].Value);

                        bool regenerate = false;
                        for (int j = 0; j < code.Count; j++)
                        {
                            if (lines[i + 1 + j].Trim() != code[j])
                            {
                                regenerate = true;
                                break;
                            }
                        }
                        if (!regenerate && !endRegionRegex.Match(lines[i + 1 + code.Count]).Success)
                            regenerate = true;

                        if (regenerate)
                        {
                            // clear content
                            Match endRegionMatch = endRegionRegex.Match(lines[i + 1]);
                            while (!endRegionMatch.Success)
                            {
                                lines.RemoveAt(i + 1);
                                endRegionMatch = endRegionRegex.Match(lines[i + 1]);
                            }

                            // add regenerated lines
                            int count = i + 1;
                            foreach (var codeLine in code)
                                lines.Insert(count++, indent + codeLine);

                            result = EResult.Success;
                        }
                    }
                    else // if there isn't such generator then remove region
                    {
                        lines.RemoveAt(i + 1);
                        lines.RemoveAt(i);
                        i--;
                    }
                }
            }

            if (Program.IsDebugging)
                file = "1" + Path.GetFileNameWithoutExtension(file) + ".txt";

            if (result == EResult.Success)
                File.WriteAllLines(file, lines.ToArray());

            return result;
        }

    }
}
