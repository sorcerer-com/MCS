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

        public static List<uint> SelectedElements = new List<uint>();

        #region Commands

        // ToolBar commands
        public ICommand NewSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    // TODO: is saved?
                    this.Engine.SceneManager.New();
                });
            }
        }

        public ICommand OpenSceneCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    OpenFileDialog ofd = new OpenFileDialog();
                    ofd.InitialDirectory = System.Environment.CurrentDirectory + "\\Scenes";
                    ofd.Filter = "My Level Files (*.mlv)|*.mlv|All Files (*.*)|*.*";
                    ofd.DefaultExt = "mlv";
                    ofd.RestoreDirectory = true;
                    if (ofd.ShowDialog() == true)
                    {
                        // TODO: is saved?
                        if (!this.Engine.SceneManager.Load(ofd.FileName))
                            ExtendedMessageBox.Show("Cannot open scene file: \n " + ofd.FileName + "!", "Open scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
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
                    // TODO: if there isn't path to the scene or Save As - ask
                    SaveFileDialog sfd = new SaveFileDialog();
                    sfd.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Levels";
                    sfd.Filter = "My Level Files (*.mlv)|*.mlv|All Files (*.*)|*.*";
                    sfd.DefaultExt = "mlv";
                    sfd.RestoreDirectory = true;
                    sfd.OverwritePrompt = true;
                    if (sfd.ShowDialog() == true)
                        if (!this.Engine.SceneManager.Save(sfd.FileName))
                            ExtendedMessageBox.Show("Cannot save scene to file: \n " + sfd.FileName + "!", "Save scene", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                });
            }
        }

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); }); }
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

            ConfigManager.LoadConfig();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // TODO: init renders
            this.KeyDown += WindowsManager.Window_KeyDown;
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.KeyDown -= WindowsManager.Window_KeyDown;

            this.Engine.Dispose();
        }
    
    }
}
