using System;
using System.IO;

namespace CodeGenerator
{
    public class Program
    {        
        public static bool IsDebugging
        {
            get { return !Console.IsInputRedirected; }
        }


        public static void Main(string[] args)
        {
            if (args.Length == 0)
                args = new string[] { Path.Combine(Directory.GetCurrentDirectory(), "Engine") };
                //args = new string[] { Path.Combine(Directory.GetCurrentDirectory(), "Engine", "Mesh.h") };

            if (args.Length != 1)
            {
                Console.WriteLine("CodeGenerator: No Arguments");
                Console.WriteLine();
                Environment.Exit(1);
            }

            args[0] = args[0].Trim('"');

            if (File.Exists(args[0]) && Path.GetExtension(args[0]) == ".h")
            {
                Console.Write("CodeGenerator: ");

                string file = args[0];
                CodeGenerator cg = new CodeGenerator(file);
                Console.Write(string.Format("Proccessing {0}... ", Path.GetFileNameWithoutExtension(file)));
                var result = cg.Proccess();

                foreach (var res in result)
                    Console.Write(res + " ");
                Console.WriteLine();
            }
            else if (Directory.Exists(args[0]))
            {
                Console.WriteLine("CodeGenerator:");

                int changes = 0;
                string[] files = Directory.GetFiles(args[0], "*.h", SearchOption.AllDirectories);
                foreach (var file in files)
                {
                    CodeGenerator cg = new CodeGenerator(file);
                    Console.Write(string.Format("\tProccessing {0}... ", Path.GetFileNameWithoutExtension(file)).PadRight(35));
                    var result = cg.Proccess();

                    foreach (var res in result)
                        Console.Write(res.ToString().PadRight(10));
                    Console.WriteLine();

                    if (result.LastIndexOf(EResult.Success) > 0)
                        changes++;
                }

                Console.WriteLine("CodeGenerator:  Done ({0} changes)", changes);
                Console.WriteLine();
            }
            else
            {
                Console.WriteLine("CodeGenerator: Invalid Argument");
                Console.WriteLine();
                Environment.Exit(1);
            }

            if (Program.IsDebugging)
                Console.ReadKey();
        }


        public static void Log(string log)
        {
            Console.WriteLine("\t\t" + log);
        }
    }
}
