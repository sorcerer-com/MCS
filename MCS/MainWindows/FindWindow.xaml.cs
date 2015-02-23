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
    public partial class FindWindow : Window, INotifyPropertyChanged
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

        public ObservableCollection<FindGridRow> Rows
        {
            get
            {
                ObservableCollection<FindGridRow> rows = new ObservableCollection<FindGridRow>();

                List<MSceneElement> mses = this.sceneManager.Elements;
                foreach (MSceneElement mse in mses)
                {
                    if (mse.Type == ESceneElementType.SystemObject)
                        continue;

                    if (!mse.Name.ToLower().Contains(this.FindText.ToLower()))
                        continue;

                    FindGridRow row = new FindGridRow();
                    row.Number = rows.Count;
                    row.ID = mse.ID;
                    row.Name = mse.Name;
                    row.Type = mse.Type.ToString();
                    row.Content = mse.Content != null ? mse.Content.ToString() : "None";
                    if (MainWindow.SelectedElements.Contains(mse.ID))
                        row.IsSelected = true;

                    rows.Add(row);
                }
                return rows;
            }
        }

        public string FindText { get; set; }

        private MSceneManager sceneManager;


        public FindWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.FindText = string.Empty;
            this.DataContext = this;
            this.sceneManager = sceneManager;

            DispatcherTimer timer = new DispatcherTimer();
            timer.Interval = new TimeSpan(0, 0, 0, 3);
            timer.Tick += new EventHandler(timer_Tick);
            timer.Start();
            timer_Tick(null, null);
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            this.OnPropertyChanged("Rows");
        }

        private void DataGrid_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
            if (mainWindow == null)
                return;

            foreach (FindGridRow item in e.RemovedItems)
                mainWindow.SelectElement(item.ID, true);

            foreach (FindGridRow item in e.AddedItems)
                if (!MainWindow.SelectedElements.Contains(item.ID))
                    mainWindow.SelectElement(item.ID);
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }
    }
}
