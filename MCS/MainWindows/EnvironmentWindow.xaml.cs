using MyEngine;
using System;
using System.Collections.Generic;
using System.Windows;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for EnvironmentWindow.xaml
    /// </summary>
    public partial class EnvironmentWindow : Window
    {
        private MSceneManager sceneManager;

        // TODO: SkyBox property item didn't shows up when SkyBox is null
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

            this.DataContext = sceneManager;
            this.sceneManager = sceneManager;
        }
    }
}
