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
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow), o); }); }
        }

        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), o); }); }
        }

        #endregion


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Scene = new MScene();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // TODO: init renders
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.Scene.Dispose();
            WindowsManager.Close();
        }
    
    }
}
