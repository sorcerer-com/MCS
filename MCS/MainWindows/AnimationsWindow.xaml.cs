using MCS.Controls;
using MCS.Dialogs;
using MCS.Managers;
using MyEngine;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Input;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for AnimationWindow.xaml
    /// </summary>
    public partial class AnimationsWindow : Window, INotifyPropertyChanged
    {
        private MAnimationManager animationManager;


        public ObservableCollection<string> Animations
        {
            get
            {
                return new ObservableCollection<string>(this.animationManager.AnimationsNames);
            }
        }

        private string selectedAnimation;
        public string SelectedAnimation
        {
            get { return this.selectedAnimation; }
            set
            {
                if (value != this.selectedAnimation)
                {
                    this.selectedAnimation = value; 
                    this.RefreshCurves();
                }
            }
        }

        public Dictionary<string, List<Point>> Curves { get; private set; }

        private string selectedCurve;
        public string SelectedCurve
        {
            get { return this.selectedCurve; }
            set
            {
                if (this.selectedCurve != value)
                {
                    this.selectedCurve = value;
                    this.OnPropertyChanged("SelectedCurve");
                    this.OnPropertyChanged("Value");
                }
            }
        }

        private double speed;
        public double Speed
        {
            get { return this.speed; }
            set
            {
                if (this.speed != value)
                {
                    this.speed = value;
                    this.OnPropertyChanged("Speed");
                }
            }
        }

        private double currentFrame;
        public double CurrentFrame
        {
            get { return this.currentFrame; }
            set
            {
                if (this.currentFrame != value)
                {
                    this.currentFrame = value;
                    this.OnPropertyChanged("CurrentFrame");
                    this.OnPropertyChanged("Value");
                }
            }
        }

        public double Value
        {
            get
            {
                if (string.IsNullOrEmpty(this.SelectedAnimation) || 
                    string.IsNullOrEmpty(this.SelectedCurve))
                    return 0.0;

                var curve = this.Curves[this.SelectedCurve];
                Point prev = new Point(-1.0, 0.0);
                foreach (var point in curve)
                {
                    if ((int)this.CurrentFrame <= point.X)
                    {
                        if (point.X < 0.0)
                            return 0.0;
                        double d = (point.X - prev.X);
                        double t = ((int)this.CurrentFrame - prev.X) / d;
                        return this.cubicBezier(t, prev, prev + new Vector(d / 2.0, 0), point - new Vector(d / 2.0, 0), point).Y;
                    }
                    prev = point;
                }
                return 0.0;
            }
            set { /* TODO: implement or remove */ }
        }

        public bool Autokey { get; set; }

        private Point mousePosition;
        
        #region Commands

        public ICommand AddAnimationCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string animationName = TextDialogBox.Show("Add Animation", "Name", "Animation" + this.animationManager.AnimationsNames.Count);
                    if (!string.IsNullOrEmpty(animationName))
                    {
                        if (!this.animationManager.AddAnimation(animationName))
                            ExtendedMessageBox.Show("Cannot create the animation '" + animationName + "'!", "Add Animation", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            this.OnPropertyChanged("Animations");
                    }
                });
            }
        }

        public ICommand RenameAnimationCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (string.IsNullOrEmpty(this.SelectedAnimation))
                        return;

                    string animationName = TextDialogBox.Show("Rename Animation", "Name", this.SelectedAnimation);
                    if (!string.IsNullOrEmpty(animationName))
                    {
                        if (!this.animationManager.RenameAnimation(this.SelectedAnimation, animationName))
                            ExtendedMessageBox.Show("Cannot rename animation '" + this.SelectedAnimation + "'!", "Rename Animation", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            this.OnPropertyChanged("Animations");
                    }
                });
            }
        }

        public ICommand CloneAnimationCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (string.IsNullOrEmpty(this.SelectedAnimation))
                        return;

                    string animationName = TextDialogBox.Show("Clone Animation", "New Name", "Animation" + this.animationManager.AnimationsNames.Count);
                    if (!string.IsNullOrEmpty(animationName))
                    {
                        if (!this.animationManager.CloneAnimation(this.SelectedAnimation, animationName))
                            ExtendedMessageBox.Show("Cannot clone animation '" + this.SelectedAnimation + "'!", "Clone Animation", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            this.OnPropertyChanged("Animations");
                    }
                });
            }
        }

        public ICommand DeleteAnimationCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (string.IsNullOrEmpty(this.SelectedAnimation))
                        return;

                    ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + this.SelectedAnimation + "'?", "Delete Animation", ExtendedMessageBoxButton.YesNo, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes)
                    {
                        if (!this.animationManager.DeleteAnimation(this.SelectedAnimation))
                            ExtendedMessageBox.Show("Cannot delete animation '" + this.SelectedAnimation + "'!", "Delete Animation", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            this.OnPropertyChanged("Animations");
                    }
                });
            }
        }


        public ICommand AddTrackCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (string.IsNullOrEmpty(this.SelectedAnimation))
                        return;

                    string trackName = TextDialogBox.Show("Add Track", "Name", "Track" + this.Curves.Count);
                    if (!string.IsNullOrEmpty(trackName))
                    {
                        if (!this.animationManager.AddTrack(this.SelectedAnimation, trackName, EAnimTrackType.None)) // TODO: type
                            ExtendedMessageBox.Show("Cannot create the track '" + trackName + "'!", "Add Track", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            RefreshCurves();
                    }
                });
            }
        }

        public ICommand DeleteTrackCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (string.IsNullOrEmpty(this.SelectedAnimation) ||
                        string.IsNullOrEmpty(this.SelectedCurve))
                        return;

                    string track = this.SelectedCurve;
                    if (track.Contains("["))
                        track = track.Substring(0, track.IndexOf('['));

                    ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + track + "'?", "Delete Track", ExtendedMessageBoxButton.YesNo, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes)
                    {
                        if (!this.animationManager.DeleteTrack(this.SelectedAnimation, track))
                            ExtendedMessageBox.Show("Cannot delete track '" + track + "'!", "Delete Track", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        else
                            RefreshCurves();
                    }
                }, o => !string.IsNullOrEmpty(this.SelectedAnimation) && !string.IsNullOrEmpty(this.SelectedCurve));
            }
        }

        #endregion


        public AnimationsWindow(MAnimationManager animationManager)
        {
            InitializeComponent();

            if (animationManager == null)
                throw new ArgumentNullException("animationManager");

            this.DataContext = this;
            this.animationManager = animationManager;

            this.RefreshCurves();
            this.Speed = 1.0;
            this.CurrentFrame = 0.0;

            // TODO: add functionality
            this.animationManager.AddAnimation("Animation");
            this.animationManager.AddTrack("Animation", "Track0", EAnimTrackType.MPoint);
            this.animationManager.SetKeyframe("Animation", "Track0", 0, new double[] { 0.0, 0.0, 0.0 });
            this.animationManager.SetKeyframe("Animation", "Track0", 30, new double[] { -10.0, 0.0, 0.0 });
            this.animationManager.SetKeyframe("Animation", "Track0", 60, new double[] { 5.0, 10.0, 0.0 });
            this.animationManager.SetKeyframe("Animation", "Track0", 90, new double[] { 15.0, 5.0, 0.0 });
            this.animationManager.SetKeyframe("Animation", "Track0", 120, new double[] { 0.0, 0.0, 0.0 });
            this.animationManager.SetKeyframe("Animation", "Track0", 240, new double[] { 10.0, 10.0, 10.0 });
            this.animationManager.AddTrack("Animation", "Track1", EAnimTrackType.MPoint);
            this.animationManager.SetKeyframe("Animation", "Track1", 0, new double[] { 0.0, 0.0, 0.0 });
            this.animationManager.AddTrack("Animation", "Track2", EAnimTrackType.MPoint);
            this.animationManager.SetKeyframe("Animation", "Track2", 0, new double[] { 0.0, 0.0, 0.0 });
            this.SelectedAnimation = "Animation";
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            e.Handled = true;
            if (e.Key == Key.Escape) // close window
                this.Close();
            //else if (e.Key == Key.Enter) // add keyframe
            //    this.graphViewer_MouseDoubleClick(null, null);
            else if (e.Key == Key.Left) // move cursor left
            {
                int d = 1;
                if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    d = 10;
                this.CurrentFrame -= d;
            }
            else if (e.Key == Key.Right) // move cursor right
            {
                int d = 1;
                if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    d = 10;
                this.CurrentFrame += d;
            }
            else if (e.Key == Key.Up) // go to previous curve
            {
                string prevCurve = string.Empty;
                foreach (var pair in this.Curves)
                {
                    if (this.SelectedCurve == pair.Key && !string.IsNullOrEmpty(prevCurve))
                        break;
                    prevCurve = pair.Key;
                }
                this.SelectedCurve = prevCurve;
            }
            else if (e.Key == Key.Down) // go to next curve
            {
                bool found = false;
                foreach (var pair in this.Curves)
                {
                    if (found)
                    {
                        this.SelectedCurve = pair.Key;
                        found = false;
                        break;
                    }
                    if (this.SelectedCurve == pair.Key)
                        found = true;
                }
                if (found)
                    this.SelectedCurve = new List<string>(this.Curves.Keys)[0];
            }
            //else if ((Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)) && e.Key == Key.P) // play
            //    this.playButton_Click(this.playButton, null);
            //else if ((Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)) && e.Key == Key.S) // stop
            //    this.playButton_Click(this.stopButton, null);
            //else if (Keyboard.IsKeyDown(Key.F12)) // Play / Pause
            //{
            //    if (this.timer.IsEnabled == true) // if is on play then stop
            //        this.playButton_Click(this.stopButton, null);
            //    else
            //        this.playButton_Click(this.playButton, null);
            //}
            else
                e.Handled = false;
        }


        public void RefreshCurves()
        {
            this.Curves = new Dictionary<string, List<Point>>();
            if (string.IsNullOrEmpty(this.SelectedAnimation))
                return;

            var animation = this.animationManager.GetAnimation(this.SelectedAnimation);
            foreach (var track in animation)
            {
                foreach (var keyframe in track.Value)
                {
                    for (int i = 0; i < keyframe.Value.Length; i++)
                    {
                        string curveName = track.Key;
                        if (keyframe.Value.Length > 0)
                            curveName += "[" + i + "]";
                        if (!this.Curves.ContainsKey(curveName))
                            this.Curves.Add(curveName, new List<Point>());
                        this.Curves[curveName].Add(new Point(keyframe.Key, keyframe.Value[i]));
                    }
                }
            }

            if (!string.IsNullOrEmpty(this.SelectedCurve) && !this.Curves.ContainsKey(this.SelectedCurve)) // check if selected curve is still valid
                this.SelectedCurve = string.Empty;

            this.OnPropertyChanged("Curves");
        }


        private void GraphsViewer_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var graphsViewer = sender as GraphsViewer;
            if (graphsViewer == null)
                return;

            if (e.ChangedButton == MouseButton.Left)
            {
                Point pos = e.GetPosition(this);
                string curveName = graphsViewer.GetCurveName(pos);
                if (graphsViewer.SelectedCurve == curveName)
                    graphsViewer.SelectedCurve = string.Empty;
                else
                    graphsViewer.SelectedCurve = curveName;
            }
            else if (e.ChangedButton == MouseButton.Right)
            {
                graphsViewer.Start = new Point();
                graphsViewer.Scale = new Size(2.0, 1.0);
            }
        }

        private void GraphsViewer_MouseMove(object sender, MouseEventArgs e)
        {
            var graphsViewer = sender as GraphsViewer;
            if (graphsViewer == null)
                return;

            Point pos = e.GetPosition(graphsViewer);
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                graphsViewer.CursorPosition = (pos.X - graphsViewer.HeaderSize.Width) / graphsViewer.Scale.Width - graphsViewer.Start.X;
            }
            else if (e.RightButton == MouseButtonState.Pressed)
            {
                graphsViewer.Start += (pos - this.mousePosition);
            }
            else if (e.MiddleButton == MouseButtonState.Pressed)
            {
                graphsViewer.Scale = (Size)((Point)graphsViewer.Scale + (pos - this.mousePosition) / 300);
            }
            this.mousePosition = pos;
        }

        private void GraphsViewer_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            var graphsViewer = sender as GraphsViewer;
            if (graphsViewer == null)
                return;

            double scale = 1.0;
            if (e.Delta > 0)
                scale = 1.1;
            else
                scale = 0.9;
            graphsViewer.Scale = new Size(graphsViewer.Scale.Width * scale, graphsViewer.Scale.Height);
        }

        private void GraphsViewer_ContextMenuOpening(object sender, System.Windows.Controls.ContextMenuEventArgs e)
        {
            FrameworkElement frameworkElement = sender as FrameworkElement;
            if (frameworkElement == null)
                return;

            List<object> items = new List<object>();
            foreach (var item in frameworkElement.ContextMenu.Items)
                items.Add(item);

            frameworkElement.ContextMenu.Items.Clear();
            foreach (var item in items)
                frameworkElement.ContextMenu.Items.Add(item);
        }


        // https://en.wikipedia.org/wiki/B%C3%A9zier_curve
        private Point bezier(double t, Point p0, Point p1)
        {
            return (Point)((Vector)p0 * (1 - t) + (Vector)p1 * t);
        }

        private Point quadraticBezier(double t, Point p0, Point p1, Point p2)
        {
            return bezier(t, bezier(t, p0, p1), bezier(t, p1, p2));
        }

        private Point cubicBezier(double t, Point p0, Point p1, Point p2, Point p3)
        {
            return bezier(t, quadraticBezier(t, p0, p1, p2), quadraticBezier(t, p1, p2, p3));
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }
}
