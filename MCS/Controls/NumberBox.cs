using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace MCS.Controls
{
    public class NumberBox : TextBox
    {
        public static DependencyProperty ValueProperty =
            DependencyProperty.Register("Value", typeof(double), typeof(NumberBox), new PropertyMetadata(0.0, OnValuePropertyChanged));

        public static RoutedEvent ChangedEvent = 
            EventManager.RegisterRoutedEvent("Changed", RoutingStrategy.Bubble, typeof(RoutedEventHandler), typeof(NumberBox));

        
        public double Value
        { 
            get { return (double)GetValue(ValueProperty); }
            set { SetValue(ValueProperty, value); }
        }

        private bool isInteger;
        public bool IsInteger
        {
            get { return this.isInteger; }
            set
            {
                this.isInteger = value;
                if (!value)
                    this.Text = this.Value.ToString(this.StringFormat, CultureInfo.InvariantCulture.NumberFormat);
                else
                    this.Text = this.Value.ToString();
            }
        }

        public double Min { get; set; }
        public double Max { get; set; }

        public bool IsMouseChangeable { get; set; }

        private string stringFormat;
        public string StringFormat
        {
            get { return this.stringFormat; }
            set
            {
                this.stringFormat = value;
                if (!this.IsInteger)
                    this.Text = this.Value.ToString(this.StringFormat, CultureInfo.InvariantCulture.NumberFormat);
                else
                    this.Text = this.Value.ToString();
            }
        }


        private DispatcherTimer timer;
        private Point mousePos;


        public NumberBox()
        {
            this.IsInteger = false;
            this.Min = double.MinValue;
            this.Max = double.MaxValue;
            this.IsMouseChangeable = true;
            this.StringFormat = "0.000";

            this.timer = new DispatcherTimer();
            this.timer.Interval = new TimeSpan(0, 0, 0, 0, 30);
            this.timer.Tick += new EventHandler(timer_Tick);

            this.TextChanged += NumberBox_TextChanged;
            this.MouseWheel += NumberBox_MouseWheel;
            this.MouseMove += NumberBox_MouseMove;
        }


        private bool raiseChangedEvent;
        public void SetValueWithoutRaiseChanged(double value)
        {
            this.raiseChangedEvent = false;
            this.Value = value;
            this.raiseChangedEvent = true;
        }

        public event RoutedEventHandler Changed
        {
            add { AddHandler(ChangedEvent, value); }
            remove { RemoveHandler(ChangedEvent, value); }
        }

        protected virtual void OnChanged()
        {
            double value = 0.0;
            if (double.TryParse(this.Text, NumberStyles.Float, CultureInfo.InvariantCulture.NumberFormat, out value))
                this.Value = value;

            if (!raiseChangedEvent)
                return;

            RoutedEventArgs args = new RoutedEventArgs(ChangedEvent, this);
            RaiseEvent(args);
        }

        private static void OnValuePropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs e)
        {
            NumberBox nb = source as NumberBox;
            if (nb == null || (e.OldValue != null && e.OldValue.Equals(e.NewValue)))
                return;

            if (!nb.IsInteger)
                nb.Text = ((double)e.NewValue).ToString(nb.StringFormat, CultureInfo.InvariantCulture.NumberFormat);
            else
                nb.Text = ((double)e.NewValue).ToString("0");
        }

        
        private void NumberBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            this.OnChanged();
            e.Handled = true;
        }

        private void NumberBox_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (!this.IsMouseChangeable)
                return;

            int lines = e.Delta / Mouse.MouseWheelDeltaForOneLine;

            if (!this.IsInteger)
            {
                double d = (double)lines / 10;
                if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                    d *= 10;
                else if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    d /= 10;
                this.Value += d;

                if (this.Value < this.Min)
                    this.Value = this.Min;
                else if (this.Value > this.Max)
                    this.Value = this.Max;
            }
            else
            {
                double d = (double)lines;
                if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                    d *= 10;
                this.Value += d;
            }

            e.Handled = true;
        }

        private void NumberBox_MouseMove(object sender, MouseEventArgs e)
        {
            if (!this.IsMouseChangeable)
                return;

            if (e.LeftButton == MouseButtonState.Pressed && !this.timer.IsEnabled)
            {
                this.mousePos = e.GetPosition(null);
                this.timer.Start();
            }
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            if (Mouse.LeftButton != MouseButtonState.Pressed)
            {
                this.timer.Stop();
                return;
            }

            double d = Mouse.GetPosition(null).Y - this.mousePos.Y;
            if (d == 0)
                return;

            if (!this.IsInteger)
            {
                if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                    d *= 10;
                else if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
                    d /= 10;
                this.Value -= (d / 10);

                if (this.Value < this.Min)
                    this.Value = this.Min;
                else if (this.Value > this.Max)
                    this.Value = this.Max;
            }
            else
            {
                this.Value -= (int)(d / 5);
            }
            
            this.mousePos = Mouse.GetPosition(null);
        }
    }
}
