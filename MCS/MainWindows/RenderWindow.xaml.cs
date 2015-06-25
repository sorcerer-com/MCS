using MCS.Managers;
using MyEngine;
using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for RenderWindow.xaml
    /// </summary>
    public partial class RenderWindow : Window
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

        public string SelectedBufferName { get; set; }

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
                return System.Windows.Interop.Imaging.CreateBitmapSourceFromHBitmap(bmp.GetHbitmap(), IntPtr.Zero, Int32Rect.Empty, System.Windows.Media.Imaging.BitmapSizeOptions.FromEmptyOptions());
            }
        }

        public MProductionRenderer.MRenderSettings RenderSettings { get; set; }

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
                        timer_Tick(null, null);
                    }
                    else
                        this.engine.ProductionRenderer.Stop();
                });
            }
        }

        #endregion


        public RenderWindow(MEngine engine)
        {
            InitializeComponent();

            if (engine == null)
                throw new ArgumentNullException("engine");

            this.DataContext = this;
            this.engine = engine;

            this.SelectedRendererType = this.RendererTypes[0];
            this.SelectedBufferName = this.BuffersNames[this.BuffersNames.Count - 1];

            this.RenderSettings = new MProductionRenderer.MRenderSettings();
            this.RenderSettings.Width = 640;
            this.RenderSettings.Height = 480;
            this.RenderSettings.RegionSize = 64;

            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 5);
            this.timer.Tick += new EventHandler(this.timer_Tick);
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            // Update Image
            // TODO: may be INotifyPropertyChanged
            this.bufferImage.GetBindingExpression(System.Windows.Controls.Image.SourceProperty).UpdateTarget();

            if (!this.engine.ProductionRenderer.IsStarted)
                this.timer.Stop();
        }

        private void Window_MouseMove(object sender, MouseEventArgs e)
        {
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
            var tg = this.bufferImage.RenderTransform as TransformGroup;
            var st = tg.Children[0] as ScaleTransform;
            double zoom = e.Delta > 0 ? 02 : 0.5;
            if (st.ScaleX > 0.1) st.ScaleX *= zoom;
            if (st.ScaleY > 0.1) st.ScaleY *= zoom;
        }

    }
}


