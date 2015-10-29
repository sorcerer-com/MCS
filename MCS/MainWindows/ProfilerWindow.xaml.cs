using MyEngine;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows;
using System.Windows.Threading;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for ProfilerWindow.xaml
    /// </summary>
    public partial class ProfilerWindow : Window, INotifyPropertyChanged
    {
        public static readonly int MaxCount = 10;
        
        public Dictionary<string, List<Point>> Curves { get; private set; }

        public Point Start { get; private set; }

        private DispatcherTimer timer;
        private double time;

        // TODO: may be list of values only, not graphic
        public ProfilerWindow()
        {
            InitializeComponent();

            this.DataContext = this;
            this.Curves = new Dictionary<string, List<Point>>();
            this.Start = new Point();

            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 0, 100);
            this.timer.Tick += new EventHandler(this.timer_Tick);
            this.timer.Start();
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            var data = MEngine.ProfilerData;
            foreach(var d in data)
            {
                string name = d.Key.Replace("MyEngine::", "");
                if (!this.Curves.ContainsKey(name))
                    this.Curves.Add(name, new List<Point>());
                this.Curves[name].Add(new Point(time, d.Value.TotalMilliseconds));

                if (this.Curves[name].Count > ProfilerWindow.MaxCount)
                    this.Curves[name].RemoveAt(0);
            }

            time += 0.1;
            if (time > 0.1 * ProfilerWindow.MaxCount)
                this.Start = new Point(-time + 0.1 * ProfilerWindow.MaxCount, this.Start.Y);
            else
                this.Start = new Point(0.0, this.Start.Y);
            this.OnPropertyChanged("Start");
        }

        private void Window_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            double d = e.Delta > 0 ? 40 : -40;
            if (this.Start.Y + d > 0)
                return;
            this.Start = new Point(this.Start.X, this.Start.Y + d);
            this.OnPropertyChanged("Start");
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }
}
