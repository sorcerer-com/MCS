using MyEngine;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for EnvironmentWindow.xaml
    /// </summary>
    public partial class EnvironmentWindow : Window, INotifyPropertyChanged
    {
        private MSceneManager sceneManager;


        public MColor AmbientLight
        {
            get { return this.sceneManager.AmbientLight; }
            set { this.sceneManager.AmbientLight = value; }
        }

        public MColor FogColor
        {
            get { return this.sceneManager.FogColor; }
            set { this.sceneManager.FogColor = value; }
        }

        public double FogDensity
        {
            get { return this.sceneManager.FogDensity; }
            set { this.sceneManager.FogDensity = value; }
        }

        public double TimeOfDay
        {
            get { return this.sceneManager.TimeOfDay; }
            set { this.sceneManager.TimeOfDay = value; }
        }

        public MContentElement SkyBox
        {
            get { return this.sceneManager.SkyBox; }
            set { this.sceneManager.SkyBox = value; }
        }

        public MCS.Controls.PropertyGridItem.GetListDelegate GetSelectedContentElementsList
        {
            get
            {
                return () =>
                {
                    List<object> res = new List<object>();
                    var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                    foreach (var id in selectedElements)
                        res.Add(this.sceneManager.Owner.ContentManager.GetElement(id));
                    return res;
                };
            }
        }


        public EnvironmentWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.DataContext = this;
            this.sceneManager = sceneManager;

            this.sceneManager.Changed += sceneManager_Changed;
        }

        void sceneManager_Changed(MSceneManager sender, MSceneElement element)
        {
            this.OnPropertyChanged("AmbientLight");
            this.OnPropertyChanged("FogColor");
            this.OnPropertyChanged("FogDensity");
            this.OnPropertyChanged("TimeOfDay");
            this.OnPropertyChanged("SkyBox");
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }
}
