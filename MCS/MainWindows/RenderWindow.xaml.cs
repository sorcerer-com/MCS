using MCS.Managers;
using MyEngine;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for RenderWindow.xaml
    /// </summary>
    public partial class RenderWindow : Window, INotifyPropertyChanged
    {
        private MEngine engine;

        private Point mousePos;
        private DispatcherTimer timer;


        public ObservableCollection<ERendererType> RendererTypes
        {
            get
            {
                return new ObservableCollection<ERendererType>(MRenderer.ProductionRendererTypes);
            }
        }

        public ERendererType SelectedRendererType { get; set; }
        
        public ObservableCollection<string> BuffersNames
        {
            get
            {
                return new ObservableCollection<string>(this.engine.ProductionRenderer.BuffersNames);
            }
        }

        private string selectedBufferName;
        public string SelectedBufferName
        {
            get { return this.selectedBufferName; }
            set
            {
                if (!this.BuffersNames.Contains(value))
                    return;

                this.selectedBufferName = value;
                this.OnPropertyChanged("Buffer");
            }
        }

        public ImageSource Buffer
        {
            get
            {
                System.Drawing.Bitmap bmp = this.engine.ProductionRenderer.GetBuffer(this.SelectedBufferName);
                if (bmp == null)
                {
                    bmp = new System.Drawing.Bitmap(1, 1);
                    bmp.SetPixel(0, 0, System.Drawing.Color.Black);
                }
                else
                {
                    // add active regions
                    System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(bmp);
                    System.Drawing.Pen pen = new System.Drawing.Pen(System.Drawing.Color.White);
                    var activeRegions = this.engine.ProductionRenderer.ActiveRegions;
                    foreach (var region in activeRegions)
                    {
                        int dx = region.Width / 10;
                        g.DrawLine(pen, region.X, region.Y, region.X + dx, region.Y);
                        g.DrawLine(pen, region.X + region.Width - 1, region.Y, region.X + region.Width - 1 - dx, region.Y);
                        g.DrawLine(pen, region.X, region.Y + region.Height - 1, region.X + dx, region.Y + region.Height - 1);
                        g.DrawLine(pen, region.X + region.Width - 1, region.Y + region.Height - 1, region.X + region.Width - 1 - dx, region.Y + region.Height - 1);
                        int dy = region.Height / 10;
                        g.DrawLine(pen, region.X, region.Y, region.X, region.Y + dy);
                        g.DrawLine(pen, region.X, region.Y + region.Height - 1, region.X, region.Y + region.Height - 1 - dy);
                        g.DrawLine(pen, region.X + region.Width - 1, region.Y, region.X + region.Width - 1, region.Y + dy);
                        g.DrawLine(pen, region.X + region.Width - 1, region.Y + region.Height - 1, region.X + region.Width - 1, region.Y + region.Height - 1 - dy);
                    }
                }

                var bitmapSource = new System.Windows.Media.Imaging.BitmapImage();
                bitmapSource.BeginInit();
                MemoryStream memoryStream = new MemoryStream();
                bmp.Save(memoryStream, System.Drawing.Imaging.ImageFormat.Bmp);
                memoryStream.Seek(0, System.IO.SeekOrigin.Begin);
                bitmapSource.StreamSource = memoryStream;
                bitmapSource.EndInit();
                return bitmapSource;
            }
        }

        private static MProductionRenderer.MRenderSettings renderSettings;
        public MProductionRenderer.MRenderSettings RenderSettings
        {
            get { return RenderWindow.renderSettings; }
        }

        #region Commands

        public ICommand RenderCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (!this.engine.ProductionRenderer.IsStarted)
                    {
                        this.engine.ProductionRenderer.Init(this.RenderSettings);
                        this.engine.ProductionRenderer.Start();
                        this.timer.Start();
                    }
                    else
                        this.engine.ProductionRenderer.Stop();
                });
            }
        }
        public string RenderCommandTooltip
        {
            get { return "Render " + WindowsManager.GetHotkey(this.GetType(), "RenderCommand", true); }
        }

        public ICommand SaveBufferCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MainWindow mainWindow = this.Owner as MainWindow;
                    if (mainWindow == null)
                        return;

                    string sceneName = Path.GetFileNameWithoutExtension(mainWindow.SceneFilePath);
                    if (string.IsNullOrEmpty(sceneName)) 
                        sceneName = "Scene";

                    System.Drawing.Bitmap bmp = this.engine.ProductionRenderer.GetBuffer(this.SelectedBufferName);
                    if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                    {
                        string name = "ScreenShots\\" + sceneName;
                        int count = 0;
                        while (File.Exists(name + count + ".png"))
                            count++;
                        bmp.Save(name + count + ".png", System.Drawing.Imaging.ImageFormat.Png);
                    }
                    else
                        bmp.Save("ScreenShots\\" + sceneName + ".png", System.Drawing.Imaging.ImageFormat.Png);
                });
            }
        }
        public string SaveBufferCommandTooltip
        {
            get { return "Save Buffer " + WindowsManager.GetHotkey(this.GetType(), "SaveBufferCommand", true); }
        }

        #endregion


        static RenderWindow()
        {
            RenderWindow.renderSettings = new MProductionRenderer.MRenderSettings();
            RenderWindow.renderSettings.Width = 640;
            RenderWindow.renderSettings.Height = 480;
            RenderWindow.renderSettings.RegionSize = 64;
            RenderWindow.renderSettings.MinSamples = 1;
            RenderWindow.renderSettings.MaxSamples = 4;
            RenderWindow.renderSettings.SamplesThreshold = 0.01;
            RenderWindow.renderSettings.MaxLights = 8;
            RenderWindow.renderSettings.MaxDepth = 8;
        }

        public RenderWindow(MEngine engine)
        {
            InitializeComponent();

            if (engine == null)
                throw new ArgumentNullException("engine");

            this.DataContext = this;
            this.engine = engine;

            this.SelectedRendererType = this.RendererTypes[0];
            this.SelectedBufferName = this.BuffersNames[this.BuffersNames.Count - 1];
            
            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 0, 100);
            this.timer.Tick += new EventHandler(this.timer_Tick);
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            // Update Image
            this.OnPropertyChanged("Buffer");

            if (!this.engine.ProductionRenderer.IsStarted)
                this.timer.Stop();
        }

        private void Window_MouseMove(object sender, MouseEventArgs e)
        {
            if (!(e.OriginalSource is System.Windows.Controls.Image ||
                  e.OriginalSource is System.Windows.Controls.Border))
                return;

            var tg = this.bufferImage.RenderTransform as TransformGroup;
            var tt = tg.Children[1] as TranslateTransform;

            if (e.LeftButton == MouseButtonState.Pressed)
            {
                Vector v = Mouse.GetPosition(null) - mousePos;
                tt.X += v.X;
                tt.Y += v.Y;
            }

            this.mousePos = Mouse.GetPosition(null);
        }
        
        private void Window_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (!(e.OriginalSource is System.Windows.Controls.Image ||
                  e.OriginalSource is System.Windows.Controls.Border))
                return;

            var tg = this.bufferImage.RenderTransform as TransformGroup;
            var st = tg.Children[0] as ScaleTransform;
            double zoom = e.Delta > 0 ? 2.0 : 0.5;
            if (st.ScaleX > 0.1) st.ScaleX *= zoom;
            if (st.ScaleY > 0.1) st.ScaleY *= zoom;
        }

        private void Window_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (!(e.OriginalSource is System.Windows.Controls.Image ||
                  e.OriginalSource is System.Windows.Controls.Border))
                return;

            var tg = this.bufferImage.RenderTransform as TransformGroup;
            var st = tg.Children[0] as ScaleTransform;
            if (e.ChangedButton == MouseButton.Left)
            {
                ImageSource buffer = this.Buffer;
                st.ScaleX = buffer.Width / this.bufferImage.ActualWidth;
                st.ScaleY = buffer.Height / this.bufferImage.ActualHeight;
            }
            else if (e.ChangedButton == MouseButton.Right)
            {
                st.ScaleX = 1.0;
                st.ScaleY = 1.0;
                var tt = tg.Children[1] as TranslateTransform;
                tt.X = 0.0;
                tt.Y = 0.0;
            }

        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
    }
}


