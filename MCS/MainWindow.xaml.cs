using MCS.Dialogs;
using MCS.MainWindows;
using MCS.Managers;
using Microsoft.Win32;
using MyEngine;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Input;

// 27.10.2014
namespace MCS
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        public enum ECursorType
        {
            Select,
            Move,
            Rotate,
            Scale
        }


        private MEngine engine;

        private Point lastMousePosition;
        private ECursorType selectedCursor;


        public bool SceneSaved { get; private set; }
        public string SceneFilePath { get; private set; }

        public MSceneElement SelectedElement
        {
            get
            {
                var selectedElements = MSelector.Elements(MSelector.ESelectionType.SceneElement);
                if (selectedElements.Count > 0)
                    return this.engine.SceneManager.GetElement(selectedElements[0]);

                return this.engine.SceneManager.ActiveCamera;
            }
        }

        public MCS.Controls.PropertyGrid.GetListDelegate GetSelectedContentElementsList
        {
            get
            {
                return (s) =>
                    {
                        List<object> res = new List<object>();
                        var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                        foreach (var id in selectedElements)
                            res.Add(this.engine.ContentManager.GetElement(id));
                        return res;
                    };
            }
        }

        public string InfoLabelContent
        {
            get
            {
                var selectedElements = MSelector.Elements(MSelector.ESelectionType.SceneElement);
                if (selectedElements.Count == 1)
                    return string.Format("'{0}' object selected", this.SelectedElement.Name);
                else
                    return string.Format("{0} objects selected", selectedElements.Count);
            }
        }
        
        #region SnapDropDown

        public string SnapDropDownImage
        {
            get
            {
                if (selectedCursor == ECursorType.Move)
                    return "/Images/MainWindow/move.png";
                else if (selectedCursor == ECursorType.Rotate)
                    return "/Images/MainWindow/rotate.png";
                else if (selectedCursor == ECursorType.Scale)
                    return "/Images/MainWindow/scale.png";

                return "/Images/MainWindow/select.png";
            }
        }

        public double[] SnapDropDownItems
        {
            get
            {
                if (selectedCursor == ECursorType.Move)
                    return new double[] { 0.1, 0.5, 1.0, 5.0, 10.0, 50.0};
                else if (selectedCursor == ECursorType.Rotate)
                    return new double[] { 1.0, 5.0, 15.0, 45.0, 90.0 };
                else if (selectedCursor == ECursorType.Scale)
                    return new double[] { 1.0, 10.0, 30.0, 50.0 };
                return new double[] { 1.0 };
            }
        }

        private double[] snapDropDownSelectedItem;
        public double SnapDropDownSelectedItem
        {
            get
            {
                if (this.snapDropDownSelectedItem == null)
                    this.snapDropDownSelectedItem = new double[] { 1.0, 1.0, 1.0, 1.0 };
                return snapDropDownSelectedItem[(int)this.selectedCursor];
            }
            set
            {
                if (this.snapDropDownSelectedItem == null)
                    this.snapDropDownSelectedItem = new double[] { 1.0, 1.0, 1.0, 1.0 };
                snapDropDownSelectedItem[(int)this.selectedCursor] = value;
            }
        }

        public bool SnapDropDownIsEnabled
        {
            get { return (this.selectedCursor != ECursorType.Select); }
        }

        #endregion

        #region Commands

        // ToolBar commands
        public ICommand NewSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (this.CheckSceneSaved())
                    {
                        this.engine.SceneManager.New();
                        this.engine.SceneManager.ActiveCamera = this.engine.SceneManager.AddElement(ESceneElementType.Camera, "Camera", @"MPackage#Meshes\System\Camera") as MCamera;
                        this.engine.SceneManager.ActiveCamera.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\FlatWhite");
                        this.OnPropertyChanged("SelectedElement");

                        this.SceneSaved = true;
                        this.SceneFilePath = string.Empty;
                        this.updateTitle();
                    }
                });
            }
        }
        public string NewSceneCommandTooltip
        {
            get { return "New " + WindowsManager.GetHotkey(this.GetType(), "NewSceneCommand", true); }
        }

        public ICommand OpenSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (!this.CheckSceneSaved())
                        return;

                    OpenFileDialog ofd = new OpenFileDialog();
                    ofd.InitialDirectory = System.Environment.CurrentDirectory + "\\Scenes";
                    ofd.Filter = "My Scene Files (*.msn)|*.msn|All Files (*.*)|*.*";
                    ofd.DefaultExt = "msn";
                    ofd.RestoreDirectory = true;
                    if (ofd.ShowDialog() == true)
                    {
                        if (!this.engine.SceneManager.Load(ofd.FileName))
                        {
                            ExtendedMessageBox.Show("Cannot open scene file: \n " + ofd.FileName + "!", "Open scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                            return;
                        }

                        this.SceneSaved = true;
                        this.SceneFilePath = ofd.FileName;
                        this.updateTitle();
                        this.OnPropertyChanged("SelectedElement");
                    }
                });
            }
        }
        public string OpenSceneCommandTooltip
        {
            get { return "Open " + WindowsManager.GetHotkey(this.GetType(), "OpenSceneCommand", true); }
        }

        public ICommand SaveSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    // if there isn't path to the scene or Save As - ask
                    if (string.IsNullOrEmpty(this.SceneFilePath) || Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    {
                        SaveFileDialog sfd = new SaveFileDialog();
                        sfd.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\Scenes";
                        sfd.Filter = "My Scene Files (*.msn)|*.msn|All Files (*.*)|*.*";
                        sfd.DefaultExt = "msn";
                        sfd.RestoreDirectory = true;
                        sfd.OverwritePrompt = true;
                        if (sfd.ShowDialog() == true)
                            this.SceneFilePath = sfd.FileName;
                        else
                            return;
                    }

                    // save scene
                    if (!this.engine.SceneManager.Save(this.SceneFilePath))
                    {
                        ExtendedMessageBox.Show("Cannot save scene to file: \n " + this.SceneFilePath + "!", "Save scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        return;
                    }

                    this.SceneSaved = true;
                    this.updateTitle();
                });
            }
        }
        public string SaveSceneCommandTooltip
        {
            get { return "Save " + WindowsManager.GetHotkey(this.GetType(), "SaveSceneCommand", true); }
        }

        public ICommand ImportSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    OpenFileDialog ofd = new OpenFileDialog();
                    ofd.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                    ofd.Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
                    ofd.DefaultExt = "xml";
                    ofd.RestoreDirectory = true;
                    if (ofd.ShowDialog() == true)
                    {
                        if (!File.Exists(ofd.FileName))
                        {
                            MessageBox.Show("Cannot import scene from this file: \n " + ofd.FileName, "Import Scene", MessageBoxButton.OK, MessageBoxImage.Error);
                            return;
                        }

                        System.Xml.XmlDocument xmlDoc = new System.Xml.XmlDocument();
                        xmlDoc.Load(ofd.FileName);

                        System.Xml.XmlElement xmlRoot = xmlDoc.FirstChild as System.Xml.XmlElement;

                        var xmlChildNodes = xmlRoot.ChildNodes;
                        foreach (var xmlNode in xmlChildNodes)
                        {
                            System.Xml.XmlElement xmlElement = xmlNode as System.Xml.XmlElement;
                            string name = xmlElement.GetAttribute("Name");
                            ESceneElementType type;
                            if (!Enum.TryParse(xmlElement.GetAttribute("Type"), out type))
                                continue;
                            string content = string.Empty;
                            if (xmlElement.HasAttribute("Content"))
                                content = xmlElement.GetAttribute("Content");
                            MSceneElement mse = this.engine.SceneManager.AddElement(type, name, content) ?? this.engine.SceneManager.GetElement(name);
                            if (xmlElement.HasAttribute("Material"))
                                mse.Material = this.engine.ContentManager.GetElement(xmlElement.GetAttribute("Material"));
                            if (xmlElement.HasAttribute("DiffuseMap"))
                                mse.DiffuseMap = this.engine.ContentManager.GetElement(xmlElement.GetAttribute("DiffuseMap"));
                            if (xmlElement.HasAttribute("NormalMap"))
                                mse.NormalMap = this.engine.ContentManager.GetElement(xmlElement.GetAttribute("NormalMap"));
                            mse.Visible = bool.Parse(xmlElement.GetAttribute("Visible"));
                            mse.Position = MPoint.Parse(xmlElement.GetAttribute("Position"));
                            mse.Rotation = MPoint.Parse(xmlElement.GetAttribute("Rotation"));
                            mse.Scale = MPoint.Parse(xmlElement.GetAttribute("Scale"));

                            if (mse.Content == null)
                                mse.Content = this.engine.ContentManager.GetElement(@"MPackage#Meshes\Primitives\Cube");
                            if (mse.Material == null)
                                mse.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\FlatWhite");

                            if (xmlElement.Name == "Light")
                            {
                                MLight light = mse as MLight;
                                ELightType ltype;
                                if (Enum.TryParse(xmlElement.GetAttribute("LType"), out ltype))
                                    light.LType = ltype;
                                light.Radius = double.Parse(xmlElement.GetAttribute("Radius"));
                                light.Color = MColor.Parse(xmlElement.GetAttribute("Color"));
                                light.SpotExponent = double.Parse(xmlElement.GetAttribute("SpotExponent"));
                                light.SpotCutoff = double.Parse(xmlElement.GetAttribute("SpotCutoff"));
                                light.Intensity = double.Parse(xmlElement.GetAttribute("Intensity"));
                            }
                        }

                        this.engine.SceneManager.AmbientLight = MColor.Parse(xmlRoot.GetAttribute("AmbientLight"));
                        this.engine.SceneManager.FogColor = MColor.Parse(xmlRoot.GetAttribute("FogColor"));
                        this.engine.SceneManager.FogDensity = double.Parse(xmlRoot.GetAttribute("FogDensity"));
                        this.engine.SceneManager.ActiveCamera = this.engine.SceneManager.GetElement(xmlRoot.GetAttribute("ActiveCamera")) as MCamera;
                    }

                    this.SceneSaved = false;
                    this.updateTitle();
                });
            }
        }
        public string ImportSceneCommandTooltip
        {
            get { return "Import " + WindowsManager.GetHotkey(this.GetType(), "ImportSceneCommand", true); }
        }

        public ICommand ExportSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    SaveFileDialog sfd = new SaveFileDialog();
                    sfd.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                    sfd.Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
                    sfd.DefaultExt = "xml";
                    sfd.RestoreDirectory = true;
                    sfd.OverwritePrompt = true;
                    if (sfd.ShowDialog() == true)
                    {
                        System.Xml.XmlDocument xmlDoc = new System.Xml.XmlDocument();

                        System.Xml.XmlElement xmlRoot = xmlDoc.CreateElement("Scene");
                        xmlRoot.SetAttribute("AmbientLight", this.engine.SceneManager.AmbientLight.ToString());
                        xmlRoot.SetAttribute("FogColor", this.engine.SceneManager.FogColor.ToString());
                        xmlRoot.SetAttribute("FogDensity", this.engine.SceneManager.FogDensity.ToString());
                        xmlRoot.SetAttribute("ActiveCamera", this.engine.SceneManager.ActiveCamera.Name);
                        xmlDoc.AppendChild(xmlRoot);

                        var mses = this.engine.SceneManager.Elements;
                        foreach (var mse in mses)
                        {
                            if (mse.Type == ESceneElementType.SystemObject)
                                continue;

                            string type = mse.Type == ESceneElementType.Light ? "Light" : "SceneElement";
                            System.Xml.XmlElement xmlElement = xmlDoc.CreateElement(type);
                            xmlElement.SetAttribute("Name", mse.Name);
                            xmlElement.SetAttribute("Type", mse.Type.ToString());
                            if (mse.Content != null)
                                xmlElement.SetAttribute("Content", mse.Content.FullName);
                            if (mse.Material != null)
                                xmlElement.SetAttribute("Material", mse.Material.FullName);
                            if (mse.DiffuseMap != null)
                                xmlElement.SetAttribute("DiffuseMap", mse.DiffuseMap.FullName);
                            if (mse.NormalMap != null)
                                xmlElement.SetAttribute("NormalMap", mse.NormalMap.FullName);
                            xmlElement.SetAttribute("Visible", mse.Visible.ToString());
                            xmlElement.SetAttribute("Position", mse.Position.ToString());
                            xmlElement.SetAttribute("Rotation", mse.Rotation.ToString());
                            xmlElement.SetAttribute("Scale", mse.Scale.ToString());

                            if (mse.Type == ESceneElementType.Light)
                            {
                                MLight light = mse as MLight;
                                xmlElement.SetAttribute("LType", light.LType.ToString());
                                xmlElement.SetAttribute("Radius", light.Radius.ToString());
                                xmlElement.SetAttribute("Color", light.Color.ToString());
                                xmlElement.SetAttribute("SpotExponent", light.SpotExponent.ToString());
                                xmlElement.SetAttribute("SpotCutoff", light.SpotCutoff.ToString());
                                xmlElement.SetAttribute("Intensity", light.Intensity.ToString());
                            }

                            xmlRoot.AppendChild(xmlElement);
                        }

                        xmlDoc.Save(sfd.FileName);
                    }
                });
            }
        }
        public string ExportSceneCommandTooltip
        {
            get { return "Export " + WindowsManager.GetHotkey(this.GetType(), "ExportSceneCommand", true); }
        }

        public ICommand CursorChangedCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (o != null)
                    {
                        Enum.TryParse<ECursorType>(o.ToString(), out this.selectedCursor);

                        this.OnPropertyChanged("SnapDropDownImage");
                        this.OnPropertyChanged("SnapDropDownItems");
                        this.OnPropertyChanged("SnapDropDownSelectedItem");
                        this.OnPropertyChanged("SnapDropDownIsEnabled");
                    }
                });
            }
        }

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); }); }
        }
        public string LogWindowCommandTooltip
        {
            get { return "Log " + WindowsManager.GetHotkey(this.GetType(), "LogWindowCommand", true); }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.engine.ContentManager); }); }
        }
        public string ContentWindowCommandTooltip
        {
            get { return "Content Browser " + WindowsManager.GetHotkey(this.GetType(), "ContentWindowCommand", true); }
        }

        public ICommand FindWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(FindWindow), this.engine.SceneManager); }); }
        }
        public string FindWindowCommandTooltip
        {
            get { return "Find " + WindowsManager.GetHotkey(this.GetType(), "FindWindowCommand", true); }
        }

        public ICommand EnvironmentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(EnvironmentWindow), this.engine.SceneManager); }); }
        }
        public string EnvironmentWindowCommandTooltip
        {
            get { return "Environment " + WindowsManager.GetHotkey(this.GetType(), "EnvironmentWindowCommand", true); }
        }

        public ICommand LayersWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LayersWindow), this.engine.SceneManager); }); }
        }
        public string LayersWindowCommandTooltip
        {
            get { return "Layers " + WindowsManager.GetHotkey(this.GetType(), "LayersWindowCommand", true); }
        }

        public ICommand RenderWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(RenderWindow), this.engine); }); }
        }
        public string RenderWindowCommandTooltip
        {
            get { return "Render " + WindowsManager.GetHotkey(this.GetType(), "RenderWindowCommand", true); }
        }


        // Scene Elements commands
        public ICommand CloneElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    List<MSceneElement> newSelectedElements = new List<MSceneElement>();
                    var selectedElements = MSelector.Elements(MSelector.ESelectionType.SceneElement);
                    foreach (var id in selectedElements)
                    {
                        MSceneElement mse = this.engine.SceneManager.GetElement(id);

                        string newName = mse.Name;
                        while (char.IsDigit(newName[newName.Length - 1])) newName = newName.Substring(0, newName.Length - 1);
                        int count = 1;
                        while (this.engine.SceneManager.ContainsElement(newName + count.ToString("000"))) count++;
                        newName = newName + count.ToString("000");

                        MSceneElement newMse = this.engine.SceneManager.CloneElement(id, newName);
                        if (newMse != null)
                            newSelectedElements.Add(newMse);
                    }
                    MSelector.Clear(MSelector.ESelectionType.SceneElement);

                    foreach (MSceneElement mse in newSelectedElements)
                        MSelector.Select(MSelector.ESelectionType.SceneElement, mse.ID);

                }, (o) => { return MSelector.Count(MSelector.ESelectionType.SceneElement) != 0; });
            }
        }

        public ICommand RenameElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MSceneElement mse = this.engine.SceneManager.GetElement(MSelector.Elements(MSelector.ESelectionType.SceneElement)[0]);
                    string newName = TextDialogBox.Show("Rename", "Name", mse.Name);
                    if (!string.IsNullOrEmpty(newName) && mse.Name != newName)
                    {
                        if (!this.engine.SceneManager.RenameElement(mse.Name, newName))
                            ExtendedMessageBox.Show("Cannot rename scene element '" + mse.Name + "' to '" + newName + "'!", "Rename element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return MSelector.Count(MSelector.ESelectionType.SceneElement) == 1; });
            }
        }

        public ICommand DeleteElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    var selectedElements = MSelector.Elements(MSelector.ESelectionType.SceneElement);
                    foreach (uint id in selectedElements)
                        this.engine.SceneManager.DeleteElement(id);
                    MSelector.Clear(MSelector.ESelectionType.SceneElement);

                }, (o) => { return MSelector.Count(MSelector.ESelectionType.SceneElement) != 0; });
            }
        }

        public ICommand AddElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    Point p = this.render.ContextMenu.PointToScreen(new Point());
                    p = this.render.PointFromScreen(p);
                    MPoint dir = this.engine.ViewPortRenderer.GetDirection(p.X, p.Y);;
                    double dist = this.engine.ViewPortRenderer.GetIntesectionPoint(p.X, p.Y).Length();
                    if (dist == 0 || dist > 1000)
                        dist = 100;

                    Tuple<MContentElement, ESceneElementType> tuple = o as Tuple<MContentElement, ESceneElementType>;
                    string name = tuple != null ? tuple.Item1.Name : o.ToString();
                    int count = 1;
                    while (this.engine.SceneManager.ContainsElement(name + count.ToString("000"))) count++;
                    name = name + count.ToString("000");

                    MSceneElement mse = null;
                    if (tuple != null)
                        mse = this.engine.SceneManager.AddElement(tuple.Item2, name, tuple.Item1.ID);
                    else if (name.StartsWith("Camera"))
                        mse = this.engine.SceneManager.AddElement(ESceneElementType.Camera, name, @"MPackage#Meshes\System\Camera");
                    else if (name.StartsWith("Light"))
                        mse = this.engine.SceneManager.AddElement(ESceneElementType.Light, name, 0);

                    if (mse != null && this.engine.SceneManager.ActiveCamera != null)
                    {
                        mse.Position = this.engine.SceneManager.ActiveCamera.Position + dir * dist;
                        mse.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\FlatWhite");
                    }
                });
            }
        }

        #endregion


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.engine = new MEngine();
            this.engine.SceneManager.Changed += SceneManager_Changed;
            MSelector.SelectionChanged += MSelector_SelectionChanged;

            this.SceneSaved = true;
            this.SceneFilePath = string.Empty;
            this.lastMousePosition = new Point();
            this.selectedCursor = ECursorType.Select;

            ConfigManager.LoadConfig();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.engine.SceneManager.ActiveCamera = this.engine.SceneManager.AddElement(ESceneElementType.Camera, "Camera", @"MPackage#Meshes\System\Camera") as MCamera;
            this.engine.SceneManager.ActiveCamera.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\FlatWhite");
            this.OnPropertyChanged("SelectedElement");
            this.SceneSaved = true;
            this.updateTitle();

            this.render.Child = new System.Windows.Forms.UserControl();
            this.render.Child.Resize += (s, ee) =>
            {
                this.engine.ViewPortRenderer.ReSize((uint)this.render.Child.Width, (uint)this.render.Child.Height);
            };
            this.render.Child.MouseDown += (s, ee) =>
            {
                if (ee.Button == System.Windows.Forms.MouseButtons.Right)
                {
                    this.render.ContextMenu.IsOpen = true;
                    this.updateContextMenuItems();
                }
            };
            this.render.Child.KeyDown += render_KeyDown;
            this.render.Child.MouseMove += render_MouseMove;
            this.render.Child.MouseDoubleClick += render_MouseDoubleClick;
            this.render.Child.MouseWheel += render_MouseWheel;
            this.render.Child.MouseUp += render_MouseUp;
            this.render.ContextMenu.DataContext = this;
            this.engine.ViewPortRenderer.Init(this.render.Child.Handle);

            this.KeyDown += WindowsManager.Window_KeyDown;

            // TODO: remove:
            MSceneElement mse = this.engine.SceneManager.AddElement(ESceneElementType.DynamicObject, "test", @"MPackage#Meshes\Primitives\Cube");
            mse.Position = new MPoint(0, 0, -100);
            mse.Rotation = new MPoint(-20, -20, 0);
            mse.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\Glass");
            for (int i = 0; i < 10; i++)
            {
                mse = this.engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test1" + i, @"MPackage#Meshes\Primitives\Cube");
                mse.Position = new MPoint(50 + i * 50, 0, -100 + i * 10);
                mse.Material = this.engine.ContentManager.GetElement(@"MPackage#Materials\FlatWhite");
            }
            this.engine.SceneManager.ActiveCamera.Rotation = new MPoint(0, -20, 0);
            MLight light = this.engine.SceneManager.AddElement(ESceneElementType.Light, "light", 0) as MLight;
            light.Position = new MPoint(15, 30, -100);
            light.Color = new MColor(1.0f, 0.1f, 0.1f);
            light.Intensity = 5000;
            light = this.engine.SceneManager.AddElement(ESceneElementType.Light, "light1", 0) as MLight;
            light.Position = new MPoint(15, 30, -50);
            light.Color = new MColor(0.1f, 1.0f, 0.1f);
            light.Intensity = 500;
            //this.engine.SceneManager.FogColor = new MColor(0.5, 0.5, 0.5);
            //this.engine.SceneManager.FogDensity = 0.01;
            this.engine.SceneManager.TimeOfDay = 10;
            this.engine.SceneManager.SkyBox = this.engine.ContentManager.GetElement(@"MPackage#Textures\SkyBoxes\ElyHills");
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            // check for saving
            if (!this.CheckSceneSaved())
            {
                e.Cancel = true;
                return;
            }

            this.KeyDown -= WindowsManager.Window_KeyDown;
            WindowsManager.CloseAllWindows();

            MSelector.SelectionChanged -= MSelector_SelectionChanged;
            this.engine.SceneManager.Changed -= SceneManager_Changed;
            this.engine.Dispose();
        }

        private void SceneManager_Changed(MSceneManager sender, MSceneElement element)
        {
            this.SceneSaved = false;
            this.updateTitle();
            this.OnPropertyChanged("SelectedElement");
        }

        private void MSelector_SelectionChanged(MSelector.ESelectionType selectionType, uint id)
        {
            if (selectionType == MSelector.ESelectionType.SceneElement)
            {
                this.OnPropertyChanged("SelectedElement");
                this.OnPropertyChanged("InfoLabelContent");
            }
        }


        void render_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            // camera movement
            MCamera camera = this.engine.SceneManager.ActiveCamera;
            if (e.KeyCode == System.Windows.Forms.Keys.W && camera != null)
                camera.Move(0, 0, -1);
            else if (e.KeyCode == System.Windows.Forms.Keys.S && camera != null)
                camera.Move(0, 0, 1);
            else if (e.KeyCode == System.Windows.Forms.Keys.A && camera != null)
                camera.Move(-1, 0, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.D && camera != null)
                camera.Move(1, 0, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.E && camera != null)
                camera.Move(0, 1, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.Q && camera != null)
                camera.Move(0, -1, 0);

            // shortcuts
            else if (e.KeyCode == System.Windows.Forms.Keys.Escape) // deselect
                MSelector.Clear(MSelector.ESelectionType.SceneElement);
            else if (e.KeyCode == System.Windows.Forms.Keys.Space) // change cursors
            {
                if (this.selectedCursor == ECursorType.Select) // translate
                    this.selectedCursor = ECursorType.Move;
                else if (this.selectedCursor == ECursorType.Move) // rotate
                    this.selectedCursor = ECursorType.Rotate;
                else if (this.selectedCursor == ECursorType.Rotate) // scale
                    this.selectedCursor = ECursorType.Scale;
                else // select
                    this.selectedCursor = ECursorType.Select;
                //this.updateCursor(true); // TODO: update radio buttons somehow

                this.OnPropertyChanged("SnapDropDownImage");
                this.OnPropertyChanged("SnapDropDownItems");
                this.OnPropertyChanged("SnapDropDownSelectedItem");
                this.OnPropertyChanged("SnapDropDownIsEnabled");
            }
            else if (e.KeyCode == System.Windows.Forms.Keys.Delete) // delete scene element
            {
                if (this.DeleteElementCommand.CanExecute(null))
                    this.DeleteElementCommand.Execute(null);
            }
            else if (e.KeyCode == System.Windows.Forms.Keys.F2) // change Editor Mode
            {
                MEngine.Mode = MEngine.Mode == EEngineMode.Editor ? EEngineMode.Engine : EEngineMode.Editor;
                this.updateTitle();
            }
            else if ((Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift)) && e.KeyCode == System.Windows.Forms.Keys.C) // change active camera
            {
                if (MSelector.Count(MSelector.ESelectionType.SceneElement) > 0) // set selected
                {
                    MCamera activeCamera = this.engine.SceneManager.GetElement(MSelector.Elements(MSelector.ESelectionType.SceneElement)[0]) as MCamera;
                    if (activeCamera != null)
                        this.engine.SceneManager.ActiveCamera = activeCamera;
                }
                else // switch between cameras
                {
                    List<MSceneElement> cameras = this.engine.SceneManager.GetElements(ESceneElementType.Camera);
                    int idx = cameras.IndexOf(this.engine.SceneManager.ActiveCamera);
                    idx = (idx + 1) % cameras.Count;
                    this.engine.SceneManager.ActiveCamera = cameras[idx] as MCamera;
                }
            }

            this.OnPropertyChanged("SelectedElement");
        }

        void render_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            Point mousePosition = new Point(e.X, e.Y);
            if (lastMousePosition.X == 0.0 && lastMousePosition.Y == 0.0)
            {
                lastMousePosition = mousePosition;
                return;
            }

            double dx = lastMousePosition.X - mousePosition.X;
            double dy = lastMousePosition.Y - mousePosition.Y;
            if (dx == 0.0 && dy == 0.0)
                return;

            if (e.Button == System.Windows.Forms.MouseButtons.Left &&
                (Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt))) // rotete camera
            {
                MCamera camera =  this.engine.SceneManager.ActiveCamera;
                if (camera != null)
                    camera.Rotate(dy / 2, dx / 2, 0);
            }
            else if (e.Button == System.Windows.Forms.MouseButtons.Left) // move camera
            {
                MCamera camera = this.engine.SceneManager.ActiveCamera;
                if (camera != null)
                {
                    if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                        camera.Move(-dx / 2, dy / 2, 0);
                    else
                        camera.Move(dx / 2, 0, dy / 2);
                }
            }
            // transform object
            else if (e.Button == System.Windows.Forms.MouseButtons.Middle &&
                this.selectedCursor != ECursorType.Select &&
                MSelector.Count(MSelector.ESelectionType.SceneElement) > 0)
            {
                MPoint rot = new MPoint();
                if (this.engine.SceneManager.ActiveCamera != null)
                {
                    rot = this.engine.SceneManager.ActiveCamera.Rotation;
                    rot.X = Math.Round(rot.X / 90) * 90;
                    rot.Y = Math.Round(rot.Y / 90) * 90;
                    rot.Z = Math.Round(rot.Z / 90) * 90;
                }

                MPoint delta = new MPoint((int)-dx / 2, (int)dy / 2, 0);
                if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    delta = new MPoint((int)-dx / 2, 0, (int)-dy / 2);
                if (this.selectedCursor == ECursorType.Rotate) 
                    delta.RotateBy(new MPoint(0.0, 0.0, -90.0));
                delta.RotateBy(rot);
                delta *= this.SnapDropDownSelectedItem; // snap value by the drop down value

                var selectedElements = MSelector.Elements(MSelector.ESelectionType.SceneElement);
                foreach(var id in selectedElements)
                {
                    MSceneElement mse = this.engine.SceneManager.GetElement(id);
                    if (mse != null)
                    {
                        if (this.selectedCursor == ECursorType.Move)
                            mse.Position += delta;
                        else if (this.selectedCursor == ECursorType.Rotate)
                            mse.Rotation -= delta;
                        else if (this.selectedCursor == ECursorType.Scale)
                            mse.Scale += delta * 0.01;
                    }
                }
                this.OnPropertyChanged("SelectedElement");
            }

            this.lastMousePosition = mousePosition;
        }

        void render_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (this.engine.SceneManager.ActiveCamera != null && this.SelectedElement != null &&
                this.SelectedElement.Equals(this.engine.SceneManager.ActiveCamera))
            {
                this.OnPropertyChanged("SelectedElement");
            }
        }

        void render_MouseDoubleClick(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            uint id = this.engine.ViewPortRenderer.GetSceneElementID(e.X, e.Y);
            MSceneElement mse = this.engine.SceneManager.GetElement(id);
            bool deselect = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            if (!deselect && !(Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)))
                MSelector.Clear(MSelector.ESelectionType.SceneElement); // deselect all
            if (mse != null)
            {
                if (!deselect)
                    MSelector.Select(MSelector.ESelectionType.SceneElement, mse.ID);
                else
                    MSelector.Deselect(MSelector.ESelectionType.SceneElement, mse.ID);
            }
        }

        void render_MouseWheel(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (this.selectedCursor != ECursorType.Select &&
                (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))) // change snap value
            {
                int currIndex = Array.IndexOf(this.SnapDropDownItems, this.SnapDropDownSelectedItem);
                currIndex += Math.Sign(e.Delta);
                currIndex %= this.SnapDropDownItems.Length;
                currIndex = currIndex < 0 ? this.SnapDropDownItems.Length - 1 : currIndex; 
                this.SnapDropDownSelectedItem = this.SnapDropDownItems[currIndex];
                this.OnPropertyChanged("SnapDropDownSelectedItem");
            }
            else if (this.engine.SceneManager.ActiveCamera != null) // move camera
            {
                this.engine.SceneManager.ActiveCamera.Move(0, 0, -e.Delta / 10);
                this.OnPropertyChanged("SelectedElement");
            }
        }


        public bool CheckSceneSaved()
        {
            if (!this.SceneSaved)
            {
                ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Do you want to save changes?", "Confirm", ExtendedMessageBoxButton.YesNoCancel, ExtendedMessageBoxImage.Question);
                if (res == ExtendedMessageBoxResult.Yes)
                {
                    this.SaveSceneCommand.Execute(null);
                    return this.SceneSaved;
                }
                else if (res != ExtendedMessageBoxResult.No) // Cancel
                    return false;
            }
            return true;
        }


        private void updateTitle()
        {
            this.Title = "My Creative Studio";
            if (MEngine.Mode != EEngineMode.Editor)
                this.Title += "!";
            if (!string.IsNullOrEmpty(this.SceneFilePath))
                this.Title += " - " + Path.GetFileNameWithoutExtension(this.SceneFilePath);
            if (!this.SceneSaved)
                this.Title += "*";
        }

        private void updateContextMenuItems()
        {
            List<object> items = new List<object>();
            foreach (var item in this.render.ContextMenu.Items)
                items.Add(item);

            this.render.ContextMenu.Items.Clear();
            foreach (var item in items)
            {
                System.Windows.Controls.MenuItem menu = item as System.Windows.Controls.MenuItem;
                if (menu != null && menu.StaysOpenOnClick)
                {
                    menu.Items.Clear();
                    List<object> selectedContentElements = this.GetSelectedContentElementsList(null);
                    foreach (var element in selectedContentElements)
                    {
                        System.Windows.Controls.MenuItem mi = new System.Windows.Controls.MenuItem();
                        mi.Header = element.ToString();
                        mi.Command = this.AddElementCommand;
                        ESceneElementType type = ESceneElementType.StaticObject;
                        if (menu.Header.ToString().Contains("Dynamic"))
                            type = ESceneElementType.DynamicObject;
                        mi.CommandParameter = new Tuple<MContentElement, ESceneElementType>(element as MContentElement, type);
                        menu.Items.Add(mi);
                    }
                }
                this.render.ContextMenu.Items.Add(item);
            }
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }
    
    }
}
