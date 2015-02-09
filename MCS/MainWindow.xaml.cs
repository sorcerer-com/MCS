using MCS.Dialogs;
using MCS.MainWindows;
using MCS.Managers;
using Microsoft.Win32;
using MyEngine;
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


        private bool sceneSaved;
        private string sceneFilePath;
        private Point lastMousePosition;

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

        #region Commands

        // ToolBar commands
        public ICommand NewSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (this.checkSceneSaved())
                    {
                        this.Engine.SceneManager.New();

                        this.sceneSaved = true;
                        this.sceneFilePath = string.Empty;
                        this.updateTitle();
                    }
                });
            }
        }
        public string NewSceneCommandTooltip
        {
            get { return "New (" + WindowsManager.GetHotkey(this.GetType(), "NewSceneCommand") + ")"; }
        }

        public ICommand OpenSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (!this.checkSceneSaved())
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
            get { return "Open (" + WindowsManager.GetHotkey(this.GetType(), "OpenSceneCommand") + ")"; }
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
            get { return "Save (" + WindowsManager.GetHotkey(this.GetType(), "SaveSceneCommand") + ")"; }
        }

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); }); }
        }
        public string LogSceneCommandTooltip
        {
            get { return "Log (" + WindowsManager.GetHotkey(this.GetType(), "LogWindowCommand") + ")"; }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.Engine.ContentManager); }); }
        }
        public string ContentWindowCommandTooltip
        {
            get { return "Content Browser (" + WindowsManager.GetHotkey(this.GetType(), "ContentWindowCommand") + ")"; }
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
                    this.selectElement(null);

                    foreach (MSceneElement mse in newSelectedElements)
                        this.selectElement(mse);
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
                    this.selectElement(null);
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

                    if (mse != null)
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

            ConfigManager.LoadConfig();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.Engine.SceneManager.ActiveCamera = this.Engine.SceneManager.AddElement(ESceneElementType.Camera, "Camera", @"MPackage#Meshes\System\Camera") as MCamera;
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
            this.render.ContextMenu.DataContext = this;
            this.Engine.ViewPortRenderer.Init(this.render.Child.Handle);

            this.KeyDown += WindowsManager.Window_KeyDown;

            // TODO: remove:
            MSceneElement mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test", @"MPackage#Meshes\Primitives\Cube");
            mse.Position = new MPoint(0, 0, 100);
            mse.Rotation = new MPoint(20, 20, 0);
            mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test1", @"MPackage#Meshes\Primitives\Cube");
            mse.Position = new MPoint(50, 0, 100);
            this.Engine.SceneManager.ActiveCamera.Rotation = new MPoint(0, 20, 0);
            MLight light = this.Engine.SceneManager.AddElement(ESceneElementType.Light, "light", 0) as MLight;
            light.Position = new MPoint(15, 30, 100);
            light.Color = new MColor(1.0f, 0.0f, 0.0f);
            light.Intensity = 5000;
            light = this.Engine.SceneManager.AddElement(ESceneElementType.Light, "light1", 0) as MLight;
            light.Position = new MPoint(15, 30, 50);
            light.Color = new MColor(0.0f, 1.0f, 0.0f);
            light.Intensity = 500;
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.KeyDown -= WindowsManager.Window_KeyDown;

            // check for saving
            if (!this.checkSceneSaved())
            {
                e.Cancel = true;
                return;
            }

            this.Engine.Dispose();
        }


        void render_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            // camera movement
            MCamera camera = this.Engine.SceneManager.ActiveCamera;
            if (e.KeyCode == System.Windows.Forms.Keys.W)
                camera.Move(0, 0, 1);
            else if (e.KeyCode == System.Windows.Forms.Keys.S)
                camera.Move(0, 0, -1);
            else if (e.KeyCode == System.Windows.Forms.Keys.A)
                camera.Move(-1, 0, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.D)
                camera.Move(1, 0, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.E)
                camera.Move(0, 1, 0);
            else if (e.KeyCode == System.Windows.Forms.Keys.Q)
                camera.Move(0, -1, 0);

            // shortcuts
            else if (e.KeyCode == System.Windows.Forms.Keys.Escape) // deselect
                this.selectElement(null, true);


            System.Threading.Thread.Sleep(50);
        }

        void render_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            Point mousePosition = new Point(e.X, e.Y);

            double dx = lastMousePosition.X - mousePosition.X;
            double dy = lastMousePosition.Y - mousePosition.Y;

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
            else if (e.Button == System.Windows.Forms.MouseButtons.Left) // move
            {
                MCamera camera = this.Engine.SceneManager.ActiveCamera;
                if (camera != null) // move camera
                {
                    if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                        camera.Move(-dx / 2, dy / 2, 0);
                    else
                        camera.Move(dx / 2, 0, -dy / 2);
                }
            }

            this.lastMousePosition = mousePosition;
            System.Threading.Thread.Sleep(50);
        }

        void render_MouseDoubleClick(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            uint id = this.Engine.ViewPortRenderer.GetSceneElementID(e.X, e.Y);
            MSceneElement mse = this.Engine.SceneManager.GetElement(id);
            bool deselect = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            if (!deselect && !(Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)))
                this.selectElement(null, false); // deselect all
            this.selectElement(mse, deselect);
        }

        void render_MouseWheel(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            MCamera camera = this.Engine.SceneManager.ActiveCamera;
            if (camera != null) // move camera
            {
                camera.Move(0, 0, e.Delta / 10);
            }
            System.Threading.Thread.Sleep(50);
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

        private bool checkSceneSaved()
        {
            if (!this.sceneSaved)
            {
                ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Do you want to save changes?", "Confirm", ExtendedMessageBoxButton.YesNoCancel, ExtendedMessageBoxImage.Question);
                if (res == ExtendedMessageBoxResult.Yes)
                {
                    this.SaveSceneCommand.Execute(null);
                    return true;
                }
                else if (res != ExtendedMessageBoxResult.No) // Cancel
                    return false;
            }
            return true;
        }

        private void selectElement(MSceneElement mse, bool deselect = false)
        {
            if (mse == null) // deselect all
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
                    MainWindow.SelectedElements.Add(mse.ID);
                    //MPoint scale = mse.Scale;
                    //this.scaleValueBox.Value = (scale.X + scale.Y + scale.Z) / 3;
                    //if (MScene.SelectedElements.Count == 1)
                    //    this.objectsComboBox.SelectedItem = mse;
                }
                else // deselect
                {
                    for (int i = 0; i < MainWindow.SelectedElements.Count; i++)
                        if (MainWindow.SelectedElements[i] == mse.ID)
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
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }
    
    }
}
