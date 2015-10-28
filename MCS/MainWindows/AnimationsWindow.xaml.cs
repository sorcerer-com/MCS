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
using System.Windows.Threading;

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
                if (this.selectedAnimation != value)
                {
                    this.selectedAnimation = value;
                    this.RefreshCurves();
                    this.OnPropertyChanged("SelectedAnimation");
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

                    this.value = this.GetCurveValue(this.SelectedCurve, (int)this.CurrentFrame);
                    this.OnPropertyChanged("Value");
                    this.OnPropertyChanged("SelectedCurve");
                }
            }
        }

        private bool isPlaying;
        public bool IsPlaying
        {
            get { return this.isPlaying; }
            set { this.isPlaying = value; this.OnPropertyChanged("IsPlaying"); }
        }

        private bool isLooping;
        public bool IsLooping
        {
            get { return this.isLooping; }
            set { this.isLooping = value; this.OnPropertyChanged("IsLooping"); }
        }

        private bool isLinear;
        public bool IsLinear
        {
            get { return this.isLinear; }
            set
            {
                this.isLinear = value;

                this.value = this.GetCurveValue(this.SelectedCurve, this.CurrentFrame);
                this.OnPropertyChanged("Value");
                this.OnPropertyChanged("IsLinear");
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

        private int currentFrame;
        public int CurrentFrame
        {
            get { return this.currentFrame; }
            set
            {
                if (this.currentFrame != value)
                {
                    this.currentFrame = value;

                    this.value = this.GetCurveValue(this.SelectedCurve, this.CurrentFrame);
                    this.OnPropertyChanged("Value");
                    this.OnPropertyChanged("CurrentFrame");
                }
            }
        }

        private double value;
        public double Value
        {
            get { return this.value; }
            set
            {
                if (this.value != value)
                {
                    this.value = value;
                    if (this.Autokey)
                        this.AddKeyframe();
                    this.OnPropertyChanged("Value");
                }
            }
        }

        public bool Autokey { get; set; }

        private Point mousePosition;
        private DispatcherTimer timer;
        
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

                    var properties = getSceneElementProperties();
                    string trackName = SelectDialogBox.Show("AddTrack", "Name", new List<string>(properties.Keys));
                    if (!string.IsNullOrEmpty(trackName))
                    {
                        EAnimTrackType trackType = EAnimTrackType.None;
                        if (properties[trackName] == typeof(double) ||
                            properties[trackName] == typeof(float) ||
                            properties[trackName] == typeof(bool))
                            trackType = EAnimTrackType.Float;
                        else if (properties[trackName] == typeof(MPoint))
                            trackType = EAnimTrackType.MPoint;
                        else if (properties[trackName] == typeof(MColor))
                            trackType = EAnimTrackType.MColor;

                        if (!this.animationManager.AddTrack(this.SelectedAnimation, trackName, trackType))
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
                });
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

            MSelector.SelectionChanging += MSelector_SelectionChanging;

            this.RefreshCurves();
            this.IsPlaying = false;
            this.IsLooping = false;
            this.Speed = 1.0;
            this.CurrentFrame = 0;

            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 0, 100);
            this.timer.Tick += new EventHandler(this.timer_Tick);
            this.timer.Start();
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.timer.Stop();
            this.animationManager.ResetTime();
            foreach(var id in MSelector.Elements(MSelector.ESelectionType.SceneElement))
            {
                this.animationManager.StopAnimation(id);
            }
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            e.Handled = true;
            if (e.Key == Key.Escape) // close window
                this.Close();
            else if (e.Key == Key.Enter) // add keyframe
                this.AddKeyframe();
            else if (e.Key == Key.Delete) // remove keyframe
                this.RemoveKeyframe();
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
            else if ((Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)) && e.Key == Key.P) // play
                this.IsPlaying = true;
            else if ((Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)) && e.Key == Key.S) // stop
                this.IsPlaying = false;
            else if (Keyboard.IsKeyDown(Key.Space)) // Play / Pause
                this.IsPlaying = !this.IsPlaying;
            else
                e.Handled = false;
        }

        void MSelector_SelectionChanging(MSelector.ESelectionType selectionType, uint id)
        {
            if (selectionType == MSelector.ESelectionType.SceneElement)
            {
                this.animationManager.StopAnimation(id);
            }
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(this.SelectedAnimation))
                return;

            this.animationManager.ResetTime();
            foreach(var id in MSelector.Elements(MSelector.ESelectionType.SceneElement))
            {
                this.animationManager.PlayAnimation(id, this.SelectedAnimation, 0, 0, false, false, 1.0, this.IsLinear);
            }

            double deltaTime = (double)this.CurrentFrame / 30 - this.animationManager.GetTime();
            this.animationManager.MoveTime(deltaTime);

            if (this.IsPlaying)
            {
                this.CurrentFrame += (int)(0.1 * 30 * this.Speed);

                int lastFrame = 0;
                foreach (var curve in this.Curves)
                    lastFrame = Math.Max(lastFrame, (int)curve.Value[curve.Value.Count - 1].X);
                if (this.CurrentFrame > lastFrame)
                {
                    this.CurrentFrame = 0;
                    if (!this.IsLooping)
                        this.IsPlaying = false;
                }
            }
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

        public double GetCurveValue(string curveName, int frame)
        {
            if (this.Curves == null || curveName == null ||
                !this.Curves.ContainsKey(curveName))
                return 0.0;

            var curve = this.Curves[curveName];
            Point prev = new Point(-1.0, 0.0);
            foreach (var point in curve)
            {
                if (frame <= point.X)
                {
                    if (point.X < 0.0)
                        return 0.0;
                    double d = (point.X - prev.X);
                    double t = (frame - prev.X) / d;
                    if (this.IsLinear)
                        return ((1.0 - t) * (Vector)prev + t * (Vector)point).Y;
                    else
                        return cubicBezier(t, prev, prev + new Vector(d / 2.0, 0), point - new Vector(d / 2.0, 0), point).Y;
                }
                prev = point;
            }
            return 0.0;
        }

        public void AddKeyframe()
        {
            if (string.IsNullOrEmpty(this.SelectedAnimation) ||
                string.IsNullOrEmpty(this.SelectedCurve))
                return;
            this.animationManager.ResetTime();

            int index = 0;
            string track = this.SelectedCurve;
            if (track.Contains("["))
            {
                index = int.Parse(track.Substring(track.IndexOf('[') + 1, track.IndexOf(']') - track.IndexOf('[') - 1));
                track = track.Substring(0, track.IndexOf('['));
            }

            double[] keyframe = new double[4];
            for (int i = 0; i < 4; i++ )
                keyframe[i] = this.GetCurveValue(string.Format("{0}[{1}]", track, i), this.CurrentFrame);
            keyframe[index] = this.Value;

            this.animationManager.SetKeyframe(this.SelectedAnimation, track, this.CurrentFrame, keyframe);
            this.RefreshCurves();
        }

        public void RemoveKeyframe()
        {
            if (string.IsNullOrEmpty(this.SelectedAnimation) ||
                string.IsNullOrEmpty(this.SelectedCurve))
                return;
            this.animationManager.ResetTime();

            string track = this.SelectedCurve;
            if (track.Contains("["))
                track = track.Substring(0, track.IndexOf('['));

            this.animationManager.RemoveKeyframe(this.SelectedAnimation, track, this.CurrentFrame);
            this.RefreshCurves();
        }


        private void GraphsViewer_MouseDown(object sender, MouseButtonEventArgs e)
        {
            var graphsViewer = sender as GraphsViewer;
            if (graphsViewer == null)
                return;

            if (e.ChangedButton == MouseButton.Left && e.ClickCount == 2)
            {
                Point pos = e.GetPosition(this);
                string curveName = graphsViewer.GetCurveName(pos);
                if (graphsViewer.SelectedCurve == curveName)
                    graphsViewer.SelectedCurve = string.Empty;
                else
                    graphsViewer.SelectedCurve = curveName;
            }
            else if (e.ChangedButton == MouseButton.Right && e.ClickCount == 2)
            {
                graphsViewer.Start = new Point();
                graphsViewer.Scale = new Size(2.0, 1.0);
            }
            graphsViewer.Focus();
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
        

        // https://en.wikipedia.org/wiki/B%C3%A9zier_curve
        private static Point bezier(double t, Point p0, Point p1)
        {
            return (Point)((Vector)p0 * (1 - t) + (Vector)p1 * t);
        }

        private static Point quadraticBezier(double t, Point p0, Point p1, Point p2)
        {
            return bezier(t, bezier(t, p0, p1), bezier(t, p1, p2));
        }

        private static Point cubicBezier(double t, Point p0, Point p1, Point p2, Point p3)
        {
            return bezier(t, quadraticBezier(t, p0, p1, p2), quadraticBezier(t, p1, p2, p3));
        }

        private static Dictionary<string, Type> getSceneElementProperties(Type type = null)
        {
            if (type == null) type = typeof(MSceneElement);
            Dictionary<string, Type> result = new Dictionary<string, Type>();

            var properties = type.GetProperties();
            foreach (var property in properties)
            {
                if (!property.CanWrite)
                    continue;

                if (property.PropertyType.IsValueType)
                    result.Add(property.Name, property.PropertyType);
                else if(property.Name == "Material" && property.PropertyType == typeof(MContentElement))
                {
                    var props = getSceneElementProperties(typeof(MMaterial));
                    foreach (var prop in props)
                        result.Add(property.Name + "." + prop.Key, prop.Value);
                }
            }

            return result;
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }
}
