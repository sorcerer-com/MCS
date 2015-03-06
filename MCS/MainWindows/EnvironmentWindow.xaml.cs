using MyEngine;
using System;
using System.Windows;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for EnvironmentWindow.xaml
    /// </summary>
    public partial class EnvironmentWindow : Window
    {
        private MSceneManager sceneManager;

        public EnvironmentWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.DataContext = sceneManager;
            this.sceneManager = sceneManager;

            // TODO: add time of the day, skybox
        }
    }
}
