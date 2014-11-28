using MCS.MainWindows;
using MCS.Managers;
using MEngine;
using System.ComponentModel;
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
        public MScene Scene { get; private set; }

        #region Commands

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); }); }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.Scene.ContentManager); }); }
        }

        #endregion


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Scene = new MScene();

            WindowsManager.Init();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // TODO: init renders
            this.KeyDown += WindowsManager.Window_KeyDown;
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.KeyDown -= WindowsManager.Window_KeyDown;

            this.Scene.Dispose();
        }
    
    }
}
