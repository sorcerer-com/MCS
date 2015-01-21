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

        public MSceneElement SelectedElement
        {
            get
            {
                if (MainWindow.SelectedElements.Count > 0)
                    return this.Engine.SceneManager.GetElement(ContentWindow.SelectedElements[0]);

                return null;
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
                    if (MainWindow.SelectedElements.Count == 0)
                        return;

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
                });
            }
        }

        public ICommand RenameElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (MainWindow.SelectedElements.Count != 1)
                        return;

                    MSceneElement mse = this.Engine.SceneManager.GetElement(MainWindow.SelectedElements[0]);
                    string newName = TextDialogBox.Show("Rename", "Name", mse.Name);
                    if (!string.IsNullOrEmpty(newName) && mse.Name != newName)
                    {
                        if (!this.Engine.SceneManager.RenameElement(mse.Name, newName))
                            ExtendedMessageBox.Show("Cannot rename scene element '" + mse.Name + "' to '" + newName + "'!", "Rename element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand DeleteElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (MainWindow.SelectedElements.Count == 0)
                        return;

                    foreach (uint id in MainWindow.SelectedElements)
                        this.Engine.SceneManager.DeleteElement(id);
                    this.selectElement(null);
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
                }
            };
            this.Engine.ViewPortRenderer.Init(this.render.Child.Handle);

            this.KeyDown += WindowsManager.Window_KeyDown;

            // TODO: remove:
            MSceneElement mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test", @"MPackage#Meshes\Primitives\Cube");
            mse.Position = new MPoint(0, 0, 100);
            mse.Rotation = new MPoint(20, 20, 0);
            //mse = this.Engine.SceneManager.AddElement(ESceneElementType.StaticObject, "test1", @"MPackage#Meshes\Cube1");
            //mse.Position = new MPoint(30, 0, 100);
            this.Engine.SceneManager.ActiveCamera.Rotation = new MPoint(0, 20, 0);
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


        private void updateTitle()
        {
            this.Title = "My Creative Studio";
            if (!string.IsNullOrEmpty(this.sceneFilePath))
                this.Title += " - " + Path.GetFileNameWithoutExtension(this.sceneFilePath);
            if (!this.sceneSaved)
                this.Title += "*";
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
            // TODO: test selection
            if (mse == null) // deselect all
            {
                MainWindow.SelectedElements.Clear();
                //this.objectsComboBox.SelectedIndex = -1;
                //this.scaleValueBox.Value = 0.0;
                //this.showProperties(mse, false);
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
                //this.showProperties(mse, false);
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
