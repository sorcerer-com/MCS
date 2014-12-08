using MCS.MainWindows;
using MCS.Managers;
using MyEngine;
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
        public MEngine Engine { get; private set; }

        #region Commands

        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow)); }); }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), this.Engine.ContentManager); }); }
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
