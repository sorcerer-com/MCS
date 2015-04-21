using MyEngine;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Threading;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for FindWindow.xaml
    /// </summary>
    public partial class FindWindow : Window
    {
        public struct FindGridRow
        {
            public int Number { get; set; }
            public uint ID { get; set; }
            public string Name { get; set; }
            public string Type { get; set; }
            public string Content { get; set; }
            public bool IsSelected { get; set; }
        }


        private MSceneManager sceneManager;


        public ObservableCollection<FindGridRow> Rows { get; private set; }

        public string FindText { get; set; }


        public FindWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.Rows = new ObservableCollection<FindGridRow>();
            this.FindText = string.Empty;
            this.DataContext = this;
            this.sceneManager = sceneManager;

            this.sceneManager.Changed += sceneManager_Changed;
            MSelector.SelectionChanged += MSelector_SelectionChanged;
            this.updateRows();
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.sceneManager.Changed -= sceneManager_Changed;
            MSelector.SelectionChanged -= MSelector_SelectionChanged;
        }

        private void MSelector_SelectionChanged(MSelector.ESelectionType selectionType, uint id)
        {
            if (selectionType == MSelector.ESelectionType.SceneElement)
                this.updateRows();
        }

        private void sceneManager_Changed(MSceneManager sender, MSceneElement element)
        {
            this.updateRows();
        }

        private void updateRows()
        {
            this.Rows.Clear();
            List<MSceneElement> mses = this.sceneManager.Elements;
            foreach (MSceneElement mse in mses)
            {
                if (mse.Type == ESceneElementType.SystemObject)
                    continue;

                if (!mse.Name.ToLower().Contains(this.FindText.ToLower()))
                    continue;

                FindGridRow row = new FindGridRow();
                row.Number = this.Rows.Count;
                row.ID = mse.ID;
                row.Name = mse.Name;
                row.Type = mse.Type.ToString();
                row.Content = mse.Content != null ? mse.Content.ToString() : "None";
                if (MSelector.IsSelected(MSelector.ESelectionType.SceneElement, mse.ID))
                    row.IsSelected = true;

                this.Rows.Add(row);
            }
        }

        private void DataGrid_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
            if (mainWindow == null)
                return;

            foreach (FindGridRow item in e.RemovedItems)
                MSelector.Deselect(MSelector.ESelectionType.SceneElement, item.ID);

            foreach (FindGridRow item in e.AddedItems)
                MSelector.Select(MSelector.ESelectionType.SceneElement, item.ID);
        }
    }
}
