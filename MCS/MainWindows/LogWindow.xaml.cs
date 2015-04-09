using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Windows.Threading;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for LogWindow.xaml
    /// </summary>
    public partial class LogWindow : Window
    {
        public struct LogGridRow
        {
            public int Number { get; set; }
            public string Type { get; set; }
            public string Category { get; set; }
            public string Description { get; set; }
        }

        public ObservableCollection<LogGridRow> Rows { get; private set; }

        private DispatcherTimer timer;

        public LogWindow()
        {
            InitializeComponent();

            this.Rows = new ObservableCollection<LogGridRow>();
            this.DataContext = this;

            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 3);
            this.timer.Tick += new EventHandler(this.timer_Tick);
            this.timer.Start();
            this.timer_Tick(null, null);
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.timer.Stop();
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            try
            {
                string[] lines = File.ReadAllLines("log.txt");
                if (this.Rows.Count == lines.Length)
                    return;
                
                this.Rows.Clear();
                foreach(string line in lines)
                {
                    string[] split = line.Split(new string[] { "[", "]: ", "\n" }, StringSplitOptions.RemoveEmptyEntries);
                    LogGridRow row = new LogGridRow();
                    row.Number = this.Rows.Count + 1;
                    row.Type = split[0].Trim();
                    row.Category = split[1].Trim();
                    row.Description = split[2].Trim();
                    this.Rows.Add(row);
                }
                if (this.Rows.Count > 0)
                    this.logDataGrid.ScrollIntoView(this.Rows[this.Rows.Count - 1]);
            }
            catch { }
        }
    }
}
