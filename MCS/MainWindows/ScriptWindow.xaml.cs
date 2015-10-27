using MCS.Managers;
using Microsoft.Build.Execution;
using MyEngine;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;


namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for ScriptWindow.xaml
    /// </summary>
    public partial class ScriptWindow : Window
    {
        private MSceneManager sceneManager;

        #region Commands

        public ICommand OpenCommand
        {
            get { return new DelegateCommand((o) => { this.openScript(); }); }
        }
        public string OpenCommandTooltip
        {
            get { return "Open " + WindowsManager.GetHotkey(this.GetType(), "OpenCommand", true); }
        }

        public ICommand UpdateCommand
        {
            get { return new DelegateCommand((o) => { this.updateScript(); }); }
        }
        public string UpdateCommandTooltip
        {
            get { return "Update " + WindowsManager.GetHotkey(this.GetType(), "UpdateCommand", true); }
        }

        #endregion

        public static string ExportPath
        {
            get
            {
                string appPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                return Path.Combine(appPath, "temp");
            }
        }


        public ScriptWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.DataContext = this;
            this.sceneManager = sceneManager;
        }

                
        private void openScript()
        {
            Directory.CreateDirectory(ScriptWindow.ExportPath);

            if (this.sceneManager.Script.Count == 0)
            {
                ScriptWindow.extractNewScript();
            }
            else
            {
                if (!ScriptWindow.extractScript(this.sceneManager.Script.ToArray()))
                    return;
            }

            if (!File.Exists(Path.Combine(ScriptWindow.ExportPath, "Script.sln")))
                return;

            Process.Start(Path.Combine(ScriptWindow.ExportPath, "Script.sln"));
        }

        private void updateScript()
        {
            using (MemoryStream memoryStream = new MemoryStream())
            {
                using (GZipStream gZip = new GZipStream(memoryStream, CompressionMode.Compress))
                {
                    List<string> files = new List<string>(Directory.GetFiles(ScriptWindow.ExportPath, "*.*", SearchOption.AllDirectories));
                    for (int i = 0; i < files.Count; i++)
                    {
                        if (files[i].StartsWith(Path.Combine(ScriptWindow.ExportPath, "obj")) ||
                            files[i].StartsWith(Path.Combine(ScriptWindow.ExportPath, "bin")))
                        {
                            files.RemoveAt(i);
                            i--;
                        }
                    }

                    // files count
                    List<byte> bytes = new List<byte>(BitConverter.GetBytes(files.Count));
                    gZip.Write(bytes.ToArray(), 0, bytes.Count);
                    foreach (var file in files)
                    {
                        string path = file.Replace(ScriptWindow.ExportPath + Path.DirectorySeparatorChar, "");
                        // path length
                        bytes = new List<byte>(BitConverter.GetBytes(path.Length));
                        gZip.Write(bytes.ToArray(), 0, bytes.Count);

                        // path
                        bytes = new List<byte>();
                        for (int i = 0; i < path.Length; i++)
                            bytes.AddRange(BitConverter.GetBytes(path[i]));
                        gZip.Write(bytes.ToArray(), 0, bytes.Count);

                        byte[] content = File.ReadAllBytes(file);
                        // content length
                        bytes = new List<byte>(BitConverter.GetBytes(content.Length));
                        gZip.Write(bytes.ToArray(), 0, bytes.Count);

                        // full path
                        gZip.Write(content, 0, content.Length);
                        gZip.Flush();
                    }
                }
                this.sceneManager.Script = new List<byte>(memoryStream.ToArray());
            }
        }


        private static void extractNewScript()
        {
            #region emptySolution
            const string emptySolution = @"
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 2013
VisualStudioVersion = 12.0.31101.0
MinimumVisualStudioVersion = 10.0.40219.1
Project(""{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"") = ""Script"", ""Script.csproj"", ""{05059C90-4B6D-4848-AE5E-9332A19B3CB8}""
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|Any CPU = Debug|Any CPU
		Release|Any CPU = Release|Any CPU
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{05059C90-4B6D-4848-AE5E-9332A19B3CB8}.Debug|Any CPU.ActiveCfg = Debug|Any CPU
		{05059C90-4B6D-4848-AE5E-9332A19B3CB8}.Debug|Any CPU.Build.0 = Debug|Any CPU
		{05059C90-4B6D-4848-AE5E-9332A19B3CB8}.Release|Any CPU.ActiveCfg = Release|Any CPU
		{05059C90-4B6D-4848-AE5E-9332A19B3CB8}.Release|Any CPU.Build.0 = Release|Any CPU
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal
";
            #endregion
            #region emptyProject
            const string emptyProject = @"<?xml version=""1.0"" encoding=""utf-8""?>
<Project ToolsVersion=""12.0"" DefaultTargets=""Build"" xmlns=""http://schemas.microsoft.com/developer/msbuild/2003"">
  <Import Project=""$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props"" Condition=""Exists('$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props')"" />
  <PropertyGroup>
    <Configuration Condition="" '$(Configuration)' == '' "">Debug</Configuration>
    <Platform Condition="" '$(Platform)' == '' "">AnyCPU</Platform>
    <ProjectGuid>{05059C90-4B6D-4848-AE5E-9332A19B3CB8}</ProjectGuid>
    <OutputType>Library</OutputType>
    <AppDesignerFolder>Properties</AppDesignerFolder>
    <RootNamespace>Script</RootNamespace>
    <AssemblyName>Script</AssemblyName>
    <TargetFrameworkVersion>v4.5</TargetFrameworkVersion>
    <FileAlignment>512</FileAlignment>
  </PropertyGroup>
  <PropertyGroup Condition="" '$(Configuration)|$(Platform)' == 'Debug|AnyCPU' "">
    <DebugSymbols>true</DebugSymbols>
    <DebugType>full</DebugType>
    <Optimize>false</Optimize>
    <OutputPath>bin\</OutputPath>
    <DefineConstants>DEBUG;TRACE</DefineConstants>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
  </PropertyGroup>
  <PropertyGroup Condition="" '$(Configuration)|$(Platform)' == 'Release|AnyCPU' "">
    <DebugType>pdbonly</DebugType>
    <Optimize>true</Optimize>
    <OutputPath>bin\</OutputPath>
    <DefineConstants>TRACE</DefineConstants>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
  </PropertyGroup>
  <ItemGroup>
    <Reference Include=""MEngine_x64"">
      <HintPath>..\MEngine_x64.dll</HintPath>
    </Reference>
    <Reference Include=""System"" />
    <Reference Include=""System.Core"" />
    <Reference Include=""System.Xml.Linq"" />
    <Reference Include=""System.Data.DataSetExtensions"" />
    <Reference Include=""Microsoft.CSharp"" />
    <Reference Include=""System.Data"" />
    <Reference Include=""System.Xml"" />
  </ItemGroup>
  <ItemGroup>
    <Compile Include=""Script.cs"" />
  </ItemGroup>
  <Import Project=""$(MSBuildToolsPath)\Microsoft.CSharp.targets"" />
  <!-- To modify your build process, add your task inside one of the targets below and uncomment it. 
       Other similar extension points exist, see Microsoft.Common.targets.
  <Target Name=""BeforeBuild"">
  </Target>
  <Target Name=""AfterBuild"">
  </Target>
  -->
</Project>";
            #endregion
            #region emptyScript
            const string emptyScript = @"using MyEngine;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Script
{
    public class Script
    {
        public static bool IsStarted { get; private set; }

        public static void Main(MEngine engine)
        {
            IsStarted = true;
        }

        public static void OnStop()
        {
            IsStarted = false;
        }
    }
}";
            #endregion

            File.WriteAllText(Path.Combine(ScriptWindow.ExportPath, "Script.sln"), emptySolution);
            File.WriteAllText(Path.Combine(ScriptWindow.ExportPath, "Script.csproj"), emptyProject);
            File.WriteAllText(Path.Combine(ScriptWindow.ExportPath, "Script.cs"), emptyScript);
        }

        private static bool extractScript(byte[] data)
        {
            using (MemoryStream memoryStream = new MemoryStream(data, false))
            {
                using (GZipStream gZip = new GZipStream(memoryStream, CompressionMode.Decompress))
                {
                    // files count
                    byte[] bytes = new byte[sizeof(int)];
                    if (gZip.Read(bytes, 0, sizeof(int)) < sizeof(int))
                        return false;
                    int filesCount = BitConverter.ToInt32(bytes, 0);
                    for (int i = 0; i < filesCount; i++)
                    {
                        // path length
                        bytes = new byte[sizeof(int)];
                        if (gZip.Read(bytes, 0, sizeof(int)) < sizeof(int))
                            return false;
                        int pathLength = BitConverter.ToInt32(bytes, 0);

                        // path
                        StringBuilder sb = new StringBuilder(pathLength);
                        bytes = new byte[sizeof(char)];
                        for (int j = 0; j < pathLength; j++)
                        {
                            if (gZip.Read(bytes, 0, sizeof(char)) < sizeof(char))
                                return false;
                            sb.Append(BitConverter.ToChar(bytes, 0));
                        }
                        string path = sb.ToString();

                        // content length
                        bytes = new byte[sizeof(int)];
                        if (gZip.Read(bytes, 0, sizeof(int)) < sizeof(int))
                            return false;
                        int contentLength = BitConverter.ToInt32(bytes, 0);

                        // content
                        bytes = new byte[contentLength];
                        gZip.Read(bytes, 0, bytes.Length);

                        Directory.CreateDirectory(Path.Combine(ScriptWindow.ExportPath, Path.GetDirectoryName(path)));
                        try
                        {
                            File.WriteAllBytes(Path.Combine(ScriptWindow.ExportPath, path), bytes);
                        }
                        catch { }
                    }
                }
            }
            return true;
        }


        private static Assembly assembly;
        private static object script;
        public static void StartScript(MEngine engine)
        {
            if (!ScriptWindow.extractScript(engine.SceneManager.Script.ToArray()))
                return;

            if (!File.Exists(Path.Combine(ScriptWindow.ExportPath, "Script.csproj")))
                return;

            BuildManager manager = BuildManager.DefaultBuildManager;
            ProjectInstance projectInstance = new ProjectInstance(Path.Combine(ScriptWindow.ExportPath, "Script.csproj"));
            var result = manager.Build(new BuildParameters(), new BuildRequestData(projectInstance, new string[] { "Build" }));
            if (result.OverallResult != BuildResultCode.Success)
                return;

            string assemblyPath = Path.Combine(ScriptWindow.ExportPath, "bin", "Script.dll");
            if (!File.Exists(assemblyPath))
                return;

            ScriptWindow.assembly = Assembly.Load(File.ReadAllBytes(assemblyPath), File.ReadAllBytes(Path.ChangeExtension(assemblyPath, "pdb")));
            Type type = ScriptWindow.assembly.GetType("Script.Script");
            if (type == null)
                return;
            ScriptWindow.script = ScriptWindow.assembly.CreateInstance("Script.Script");
            MethodInfo mainMethod = type.GetMethod("Main", new Type[] { typeof(MEngine) });
            if (mainMethod == null)
                return;

            Task.Run(() => mainMethod.Invoke(ScriptWindow.script, new object[] { engine }));
        }

        public static void StopScript()
        {
            if (ScriptWindow.script == null)
                return;

            Type type = ScriptWindow.assembly.GetType("Script.Script");
            if (type == null)
                return;
            MethodInfo stopMethod = type.GetMethod("OnStop");
            if (stopMethod == null)
                return;

            stopMethod.Invoke(ScriptWindow.script, null);
            ScriptWindow.script = null;
        }

    }
}
