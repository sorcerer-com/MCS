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
        public MEngine Engine { get; private set; }

        public static List<uint> SelectedElements = new List<uint>();

        public enum ECursorType
        {
            Select,
            Move,
            Rotate,
            Scale
        }


        private bool sceneSaved;
        private string sceneFilePath;
        private Point lastMousePosition;
        private ECursorType selectedCursor;


        public MSceneElement SelectedElement
        {
            get
            {
                if (MainWindow.SelectedElements.Count > 0)
                    return this.Engine.SceneManager.GetElement(MainWindow.SelectedElements[0]);

                return this.Engine.SceneManager.ActiveCamera;
            }
        }

        public MCS.Controls.PropertyGrid.GetListDelegate GetSelectedContentElementsList
        {
            get
            {
                return (s) =>
                    {
                        List<object> res = new List<object>();
                        foreach (var id in ContentWindow.SelectedElements)
                            res.Add(this.Engine.ContentManager.GetElement(id));
                        return res;
                    };
            }
        }

        public string InfoLabelContent
        {
            get
            {
                if (MainWindow.SelectedElements.Count == 1)
                    return string.Format("'{0}' object selected", this.SelectedElement.Name);
                else
                    return string.Format("{0} objects selected", MainWindow.SelectedElements.Count);
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
                
                return "/Images/MainWindow/scale.png";
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
                return null;
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

        public Visibility SnapDropDownVisibility
        {
            get
            {
                if (this.selectedCursor == ECursorType.Select)
                    return Visibility.Collapsed;
                return Visibility.Visible;
            }
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
                        this.Engine.SceneManager.New();
                        this.Engine.SceneManager.ActiveCamera = this.Engine.SceneManager.AddElement(ESceneElementType.Camera, "Camera", @"MPackage#Meshes\System\Camera") as MCamera;
                        this.OnPropertyChanged("SelectedElement");

                        this.sceneSaved = true;
                        this.sceneFilePath = string.Empty;
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
                        if (!this.Engine.SceneManager.Load(ofd.FileName))
                        {
                            ExtendedMessageBox.Show("Cannot open scene file: \n " + ofd.FileName + "!", "Open scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                            return;
                        }

                        this.sceneSaved = true;
                        this.sceneFilePath = ofd.FileName;
                        this.updateTitle();
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
                    if (string.IsNullOrEmpty(this.sceneFilePath) || Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    {
                        SaveFileDialog sfd = new SaveFileDialog();
                        sfd.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\Scenes";
                        sfd.Filter = "My Scene Files (*.msn)|*.msn|All Files (*.*)|*.*";
                        sfd.DefaultExt = "msn";
                        sfd.RestoreDirectory = true;
                        sfd.OverwritePrompt = true;
                        if (sfd.ShowDialog() == true)
                            this.sceneFilePath = sfd.FileName;
                        else
                            return;
                    }

                    // save scene
                    if (!this.Engine.SceneManager.Save(this.sceneFilePath))
                    {
                        ExtendedMessageBox.Show("Cannot save scene to file: \n " + this.sceneFilePath + "!", "Save scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        return;
                    }

                    this.sceneSaved = true;
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
                            MSceneElement mse = this.Engine.SceneManager.AddElement(type, name, content);
                            if (xmlElement.HasAttribute("Material"))
                                mse.Material = this.Engine.ContentManager.GetElement(xmlElement.GetAttribute("Material"));
                            mse.Visible = bool.Parse(xmlElement.GetAttribute("Visible"));
                            mse.Position = MPoint.Parse(xmlElement.GetAttribute("Position"));
                            mse.Rotation = MPoint.Parse(xmlElement.GetAttribute("Rotation"));
                            mse.Scale = MPoint.Parse(xmlElement.GetAttribute("Scale"));
                        }

                        this.Engine.SceneManager.AmbientLight = MColor.Parse(xmlRoot.GetAttribute("AmbientLight"));
                        this.Engine.SceneManager.FogColor = MColor.Parse(xmlRoot.GetAttribute("FogColor"));
                        this.Engine.SceneManager.FogDensity = double.Parse(xmlRoot.GetAttribute("FogDensity"));
                        this.Engine.SceneManager.ActiveCamera = this.Engine.SceneManager.GetElement(xmlRoot.GetAttribute("ActiveCamera")) as MCamera;
                    }

                    this.sceneSaved = false;
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
                        xmlRoot.SetAttribute("AmbientLight", this.Engine.SceneManager.AmbientLight.ToString());
                        xmlRoot.SetAttribute("FogColor", this.Engine.SceneManager.FogColor.ToString());
                        xmlRoot.SetAttribute("FogDensity", this.Engine.SceneManager.FogDensity.ToString());
                        xmlRoot.SetAttribute("ActiveCamera", this.Engine.SceneManager.ActiveCamera.Name);
                        xmlDoc.AppendChild(xmlRoot);

                        var mses = this.Engine.SceneManager.Elements;
                        foreach (var mse in mses)
                        {
                            if (mse.Type == ESceneElementType.SystemObject)
                                continue;

                            System.Xml.XmlElement xmlElement = xmlDoc.CreateElement("SceneElement");
                            xmlElement.SetAttribute("Name", mse.Name);
                            xmlElement.SetAttribute("Type", mse.Type.ToString());
                            if (mse.Content != null)
                                xmlElement.SetAttribute("Content", mse.Content.FullName);
                            if (mse.Material != null)
                                xmlElement.SetAttribute("Material", mse.Material.FullName);
                            xmlElement.SetAttribute("Visible", mse.Visible.ToString());
                            xmlElement.SetAttribute("Position", mse.Position.ToString());
                            xmlElement.SetAttribute("Rotation", mse.Rotation.ToString());
                            xmlElement.SetAttribute("Scale", mse.Scale.ToString());
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
                        this.OnPropertyChanged("SnapDropDownVisibility");
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
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.Engine.ContentManager); }); }
        }
        public string ContentWindowCommandTooltip
        {
            get { return "Content Browser " + WindowsManager.GetHotkey(this.GetType(), "ContentWindowCommand", true); }
        }

        public ICommand FindWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(FindWindow), this.Engine.SceneManager); }); }
        }
        public string FindWindowCommandTooltip
        {
            get { return "Find " + WindowsManager.GetHotkey(this.GetType(), "FindWindowCommand", true); }
        }

        public ICommand EnvironmentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(EnvironmentWindow), this.Engine.SceneManager); }); }
        }
        public string EnvironmentWindowCommandTooltip
        {
            get { return "Environment " + WindowsManager.GetHotkey(this.GetType(), "EnvironmentWindowCommand", true); }
        }


        // Scene Elements commands
        public ICommand CloneElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    List<MSceneElement> newSelectedElements = new List<MSceneElement>();
                    foreach (var id in MainWindow.SelectedElements)
                    {
                        MSceneElement mse = this.Engine.SceneManager.GetElement(id);
                        MSceneElement newMse = this.Engine.SceneManager.CloneElement(mse, mse.Name + "2");
                        newSelectedElements.Add(newMse);
                    }
                    this.SelectElement(0);

                    foreach (MSceneElement mse in newSelectedElements)
                        this.SelectElement(mse.ID);
                }, (o) => { return MainWindow.SelectedElements.Count != 0; });
            }
        }

        public ICommand RenameElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MSceneElement mse = this.Engine.SceneManager.GetElement(MainWindow.SelectedElements[0]);
                    string newName = TextDialogBox.Show("Rename", "Name", mse.Name);
                    if (!string.IsNullOrEmpty(newName) && mse.Name != newName)
                    {
                        if (!this.Engine.SceneManager.RenameElement(mse.Name, newName))
                            ExtendedMessageBox.Show("Cannot rename scene element '" + mse.Name + "' to '" + newName + "'!", "Rename element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return MainWindow.SelectedElements.Count == 1; });
            }
        }

        public ICommand DeleteElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    foreach (uint id in MainWindow.SelectedElements)
                        this.Engine.SceneManager.DeleteElement(id);
                    this.SelectElement(0);
                }, (o) => { return MainWindow.SelectedElements.Count != 0; });
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
                    MPoint dir = this.Engine.ViewPortRenderer.GetDirection(p.X, p.Y);;
                    double dist = this.Engine.ViewPortRenderer.GetIntesectionPoint(p.X, p.Y).Length();
                    if (dist == 0 || dist > 1000)
                        dist = 100;

                    MContentElement element = o as MContentElement;
                    string name = element != null ? element.Name : o.ToString();
                    int count = 0;
                    while (this.Engine.SceneManager.ContainElement(name + count)) count++;

                    MSceneElement mse = null;
                    if (element != null)
                        mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, name + count, element.ID);
                    else if (name == "Camera")
                        mse = this.Engine.SceneManager.AddElement(ESceneElementType.Camera, name + count, @"MPackage#Meshes\System\Camera");
                    else if (name == "Light")
                        mse = this.Engine.SceneManager.AddElement(ESceneElementType.Light, name + count, 0);

                    if (mse != null && this.Engine.SceneManager.ActiveCamera != null)
                        mse.Position = this.Engine.SceneManager.ActiveCamera.Position + dir * dist;
                });
            }
        }

        #endregion


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Engine = new MEngine();
            this.Engine.SceneManager.Changed += (s, ee) =>
            {
                this.sceneSaved = false;
                this.updateTitle();
            };

            this.sceneSaved = true;
            this.sceneFilePath = string.Empty;
            this.lastMousePosition = new Point();
            this.selectedCursor = ECursorType.Select;

            ConfigManager.LoadConfig();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.Engine.SceneManager.ActiveCamera = this.Engine.SceneManager.AddElement(ESceneElementType.Camera, "Camera", @"MPackage#Meshes\System\Camera") as MCamera;
            this.OnPropertyChanged("SelectedElement");
            this.sceneSaved = true;
            this.updateTitle();

            this.render.Child = new System.Windows.Forms.UserControl();
            this.render.Child.Resize += (s, ee) =>
            {
                this.Engine.ViewPortRenderer.ReSize(this.render.Child.Width, this.render.Child.Height);
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
            this.Engine.ViewPortRenderer.Init(this.render.Child.Handle);

            this.KeyDown += WindowsManager.Window_KeyDown;

            // TODO: remove:
            MSceneElement mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test", @"MPackage#Meshes\Primitives\Cube");
            mse.Position = new MPoint(0, 0, 100);
            mse.Rotation = new MPoint(20, 20, 0);
            mse.Material = this.Engine.ContentManager.GetElement(@"Apartment#Apartment\test");
            for (int i = 0; i < 10; i++)
            {
                mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test1" + i, @"MPackage#Meshes\Primitives\Cube");
                mse.Position = new MPoint(50 + i * 50, 0, 100 + i * 10);
            }
            this.Engine.SceneManager.ActiveCamera.Rotation = new MPoint(0, 20, 0);
            MLight light = this.Engine.SceneManager.AddElement(ESceneElementType.Light, "light", 0) as MLight;
            light.Position = new MPoint(15, 30, 100);
            light.Color = new MColor(1.0f, 0.1f, 0.1f);
            light.Intensity = 5000;
            light = this.Engine.SceneManager.AddElement(ESceneElementType.Light, "light1", 0) as MLight;
            light.Position = new MPoint(15, 30, 50);
            light.Color = new MColor(0.1f, 1.0f, 0.1f);
            light.Intensity = 500;
            this.Engine.SceneManager.FogColor = new MColor(0.5, 0.5, 0.5);
            this.Engine.SceneManager.FogDensity = 0.01;
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

            this.Engine.Dispose();
        }


        void render_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            // camera movement
            MCamera camera = this.Engine.SceneManager.ActiveCamera;
            if (e.KeyCode == System.Windows.Forms.Keys.W && camera != null)
                camera.Move(0, 0, 1);
            else if (e.KeyCode == System.Windows.Forms.Keys.S && camera != null)
                camera.Move(0, 0, -1);
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
                this.SelectElement(0, true);
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
                this.OnPropertyChanged("SnapDropDownVisibility");
            }

            this.OnPropertyChanged("SelectedElement");
        }

        void render_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            Point mousePosition = new Point(e.X, e.Y);

            double dx = lastMousePosition.X - mousePosition.X;
            double dy = lastMousePosition.Y - mousePosition.Y;
            if (dx == 0.0 && dy == 0.0)
                return;

            if (e.Button == System.Windows.Forms.MouseButtons.Left &&
                (Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt))) // rotete camera
            {
                MCamera camera =  this.Engine.SceneManager.ActiveCamera;
                if (camera != null)
                {
                    MPoint angle = camera.Rotation;
                    angle.X -= dy / 2;
                    angle.Y -= dx / 2;
                    camera.Rotation = angle;
                }
            }
            else if (e.Button == System.Windows.Forms.MouseButtons.Left) // move camera
            {
                MCamera camera = this.Engine.SceneManager.ActiveCamera;
                if (camera != null)
                {
                    if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                        camera.Move(-dx / 2, dy / 2, 0);
                    else
                        camera.Move(dx / 2, 0, -dy / 2);
                }
            }
            // transform object
            else if (e.Button == System.Windows.Forms.MouseButtons.Middle &&
                this.selectedCursor != ECursorType.Select &&
                MainWindow.SelectedElements.Count > 0)
            {
                MPoint rot = new MPoint();
                if (this.Engine.SceneManager.ActiveCamera != null)
                {
                    rot = this.Engine.SceneManager.ActiveCamera.Rotation;
                    rot.X = Math.Round(rot.X / 90) * 90;
                    rot.Y = Math.Round(rot.Y / 90) * 90;
                    rot.Z = Math.Round(rot.Z / 90) * 90;
                }

                MPoint delta = new MPoint((int)-dx / 2, (int)dy / 2, 0);
                if (this.selectedCursor == ECursorType.Rotate) 
                    delta.RotateBy(new MPoint(0.0, 0.0, -90.0));
                delta.RotateBy(rot);
                delta *= this.SnapDropDownSelectedItem; // snap value by the drop down value

                foreach(var id in MainWindow.SelectedElements)
                {
                    MSceneElement mse = this.Engine.SceneManager.GetElement(id);
                    if (mse != null)
                    {
                        if (this.selectedCursor == ECursorType.Move)
                            mse.Position += delta;
                        else if (this.selectedCursor == ECursorType.Rotate)
                            mse.Rotation += delta;
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
            if (this.Engine.SceneManager.ActiveCamera != null && this.SelectedElement.Equals(this.Engine.SceneManager.ActiveCamera))
            {
                this.OnPropertyChanged("SelectedElement");
            }
        }

        void render_MouseDoubleClick(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            uint id = this.Engine.ViewPortRenderer.GetSceneElementID(e.X, e.Y);
            MSceneElement mse = this.Engine.SceneManager.GetElement(id);
            bool deselect = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            if (!deselect && !(Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)))
                this.SelectElement(0, false); // deselect all
            this.SelectElement(mse.ID, deselect);
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
            else if (this.Engine.SceneManager.ActiveCamera != null) // move camera
            {
                this.Engine.SceneManager.ActiveCamera.Move(0, 0, e.Delta / 10);
                this.OnPropertyChanged("SelectedElement");
            }
        }


        public bool CheckSceneSaved()
        {
            if (!this.sceneSaved)
            {
                ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Do you want to save changes?", "Confirm", ExtendedMessageBoxButton.YesNoCancel, ExtendedMessageBoxImage.Question);
                if (res == ExtendedMessageBoxResult.Yes)
                {
                    this.SaveSceneCommand.Execute(null);
                    return this.sceneSaved;
                }
                else if (res != ExtendedMessageBoxResult.No) // Cancel
                    return false;
            }
            return true;
        }

        public void SelectElement(uint mseID, bool deselect = false)
        {
            if (mseID == 0) // deselect all
            {
                MainWindow.SelectedElements.Clear();
                //this.objectsComboBox.SelectedIndex = -1;
                //this.scaleValueBox.Value = 0.0;
                //this.selectedCursor = ECursorType.Select;
            }
            else
            {
                if (!deselect) // select
                {
                    if (!MainWindow.SelectedElements.Contains(mseID))
                        MainWindow.SelectedElements.Add(mseID);
                    //MPoint scale = mse.Scale;
                    //this.scaleValueBox.Value = (scale.X + scale.Y + scale.Z) / 3;
                    //if (MScene.SelectedElements.Count == 1)
                    //    this.objectsComboBox.SelectedItem = mse;
                }
                else // deselect
                {
                    for (int i = 0; i < MainWindow.SelectedElements.Count; i++)
                        if (MainWindow.SelectedElements[i] == mseID)
                        {
                            MainWindow.SelectedElements.RemoveAt(i);
                            break;
                        }

                    /*
                    if (MainWindow.SelectedElements.Count > 0)
                    {
                        this.objectsComboBox.SelectedItem = MScene.SelectedElements[0];
                        MPoint scale = MScene.SelectedElements[0].Scale;
                        this.scaleValueBox.Value = (scale.X + scale.Y + scale.Z) / 3;
                    }
                    else
                    {
                        this.objectsComboBox.SelectedIndex = -1;
                        this.scaleValueBox.Value = 0.0;
                        this.selectedCursor = ECursorType.Select;
                    }*/
                }
            }
            //this.updateCursor(false);

            this.OnPropertyChanged("SelectedElement");
            this.OnPropertyChanged("InfoLabelContent");
        }


        private void updateTitle()
        {
            this.Title = "My Creative Studio";
            if (!string.IsNullOrEmpty(this.sceneFilePath))
                this.Title += " - " + Path.GetFileNameWithoutExtension(this.sceneFilePath);
            if (!this.sceneSaved)
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
                if (menu != null && menu.Header.ToString() == "Add Object")
                {
                    menu.Items.Clear();
                    List<object> selectedContentElements = this.GetSelectedContentElementsList(null);
                    foreach (var element in selectedContentElements)
                    {
                        System.Windows.Controls.MenuItem mi = new System.Windows.Controls.MenuItem();
                        mi.Header = element.ToString();
                        mi.Command = this.AddElementCommand;
                        mi.CommandParameter = element;
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
