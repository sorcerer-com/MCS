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
    public partial class MainWindow : Window
    {
        public MEngine Engine { get; private set; }

        private bool sceneSaved; // TODO: on scene changed
        private string sceneFilePath;

        public static List<uint> SelectedElements = new List<uint>();

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

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); this.sceneSaved = false; this.updateTitle(); }); }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.Engine.ContentManager); }); }
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
                    /* TODO: fix selection
                    this.selectElement(null, false);

                    foreach (MSceneElement mse in newSelectedElements)
                        this.selectElement(mse, false);
                    this.saved = false;
                     */
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
                        else
                        {
                            // refresh scene elements in object list
                            MainWindow.SelectedElements.Clear();
                            //this.saved = false;
                        }
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
                    // TODO: implement
                    throw new System.NotImplementedException();
                });
            }
        }

        public ICommand PropertiesElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    // TODO: implement
                    throw new System.NotImplementedException();
                });
            }
        }

        #endregion


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Engine = new MEngine();

            this.sceneSaved = true;
            this.sceneFilePath = string.Empty;

            ConfigManager.LoadConfig();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.render.Child = new System.Windows.Forms.UserControl();
            this.render.Child.Resize += (s, ee) =>
            {
                this.Engine.ViewPortRenderer.ReSize(this.render.Child.Width, this.render.Child.Height);
            };
            this.Engine.ViewPortRenderer.Init(this.render.Child.Handle);

            this.KeyDown += WindowsManager.Window_KeyDown;
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
    
    }
}
