using MCS.Commands;
using MEngine;
using System.ComponentModel;
using System.Windows;

// 27.10.2014
namespace MCS
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MScene Scene { get; private set; }

        public MainCommands Commands { get; private set; }


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Scene = new MScene();
            this.Commands = new MainCommands();
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
