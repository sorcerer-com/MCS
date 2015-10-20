using System;
using System.Collections;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace MCS.Controls
{
    public class PropertyGridItem : Grid
    {
        public delegate List<object> GetListDelegate();

        public static readonly DependencyProperty ObjectProperty =
            DependencyProperty.Register("Object", typeof(object), typeof(PropertyGridItem), new PropertyMetadata(OnPropertyChanged));

        public static readonly DependencyProperty CanWriteProperty =
            DependencyProperty.Register("CanWrite", typeof(bool), typeof(PropertyGridItem), new PropertyMetadata(true, OnPropertyChanged));

        public static readonly DependencyProperty FloatStringFormatProperty =
            DependencyProperty.Register("FloatStringFormat", typeof(string), typeof(PropertyGridItem), new PropertyMetadata("0.000", OnPropertyChanged));

        public static readonly DependencyProperty GetListProperty =
            DependencyProperty.Register("GetList", typeof(GetListDelegate), typeof(PropertyGridItem), new PropertyMetadata(OnPropertyChanged));

        public static readonly RoutedEvent ChangedEvent =
            EventManager.RegisterRoutedEvent("Changed", RoutingStrategy.Bubble, typeof(RoutedEventHandler), typeof(PropertyGridItem));


        public object Object
        {
            get { return GetValue(ObjectProperty); }
            set { SetValue(ObjectProperty, value); }
        }

        public bool CanWrite
        {
            get { return (bool)GetValue(CanWriteProperty); }
            set { SetValue(CanWriteProperty, value); }
        }

        public string FloatStringFormat
        {
            get { return (string)GetValue(FloatStringFormatProperty); }
            set { SetValue(FloatStringFormatProperty, value); }
        }

        public GetListDelegate GetList
        {
            get { return (GetListDelegate)GetValue(GetListProperty); }
            set { SetValue(GetListProperty, value); }
        }


        public PropertyGridItem()
        {
        }

        public PropertyGridItem(object obj)
            : this()
        {
            this.Object = obj;
        }


        public event RoutedEventHandler Changed
        {
            add { AddHandler(ChangedEvent, value); }
            remove { RemoveHandler(ChangedEvent, value); }
        }

        protected virtual void OnChanged()
        {
            RoutedEventArgs args = new RoutedEventArgs(ChangedEvent, this);
            RaiseEvent(args);
        }

        private static void OnPropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs e)
        {
            PropertyGridItem pgi = source as PropertyGridItem;
            pgi.IsEnabled = pgi.CanWrite;
            if (pgi == null || (e.OldValue != null && e.OldValue.Equals(e.NewValue)))
                return;

            if (e.OldValue != null && e.NewValue != null && e.OldValue.GetType().Equals(e.NewValue.GetType()))
                pgi.Update();
            else
                pgi.Refresh();
        }


        public void Refresh()
        {
            this.ColumnDefinitions.Clear();
            this.RowDefinitions.Clear();
            this.Children.Clear();

            this.Margin = new Thickness(3);
            this.IsEnabled = CanWrite;

            Type type = this.Object != null ? this.Object.GetType() : null;
            if (type == null || this.GetList != null)
            {
                ColumnDefinition cd = new ColumnDefinition();
                this.ColumnDefinitions.Add(cd);
                cd = new ColumnDefinition();
                cd.Width = new GridLength(0, GridUnitType.Auto);
                this.ColumnDefinitions.Add(cd);
                
                TextBox tb = new TextBox();
                tb.Background = new LinearGradientBrush(Colors.Gray, Colors.White, 90);
                if (this.Object != null)
                    tb.Text = this.Object.ToString();
                else
                    tb.Text = "None";
                tb.ToolTip = tb.Text;
                tb.IsReadOnly = true;
                tb.TextChanged += new TextChangedEventHandler(value_Changed);
                this.Children.Add(tb);
                Grid.SetColumn(tb, 0);

                if (this.GetList != null)
                {
                    Button btn = new Button();
                    btn.Tag = tb;
                    btn.Content = "<";
                    btn.Width = 20;
                    btn.Margin = new Thickness(5, 0, 0, 0);
                    btn.Click += new RoutedEventHandler(button_Click);
                    this.Children.Add(btn);
                    Grid.SetColumn(btn, 1);
                }
            }
            else if (type == typeof(uint) || type == typeof(int) ||
                type == typeof(float) || type == typeof(double))
            {
                NumberBox nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                if (type == typeof(uint))
                {
                    nb.SetValueWithoutRaiseChanged((uint)this.Object);
                    nb.IsInteger = true;
                }
                else if (type == typeof(int))
                {
                    nb.SetValueWithoutRaiseChanged((int)this.Object);
                    nb.IsInteger = true;
                }
                else
                    nb.SetValueWithoutRaiseChanged((double)this.Object);
                nb.ToolTip = nb.Text;
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
            }
            else if (type == typeof(bool))
            {
                ComboBox cb = new ComboBox();
                cb.Items.Add("True");
                cb.Items.Add("False");
                cb.Text = this.Object.ToString();
                cb.SelectedItem = cb.Text;
                cb.ToolTip = cb.Text;
                cb.SelectionChanged += new SelectionChangedEventHandler(value_Changed);
                this.Children.Add(cb);
            }
            else if (type.IsEnum)
            {
                ComboBox cb = new ComboBox();
                foreach (var enm in Enum.GetNames(type))
                    cb.Items.Add(enm.ToString());
                cb.Text = this.Object.ToString();
                cb.SelectedItem = cb.Text;
                cb.ToolTip = cb.Text;
                cb.SelectionChanged += new SelectionChangedEventHandler(value_Changed);
                this.Children.Add(cb);
            }
            else if (type == typeof(MyEngine.MColor))
            {
                this.ColumnDefinitions.Add(new ColumnDefinition());
                this.ColumnDefinitions.Add(new ColumnDefinition());
                this.ColumnDefinitions.Add(new ColumnDefinition());
                this.ColumnDefinitions.Add(new ColumnDefinition());

                MyEngine.MColor color = (MyEngine.MColor)this.Object;
                Color c = Color.FromArgb((byte)(color.A * 255), (byte)(color.R * 255), (byte)(color.G * 255), (byte)(color.B * 255));

                NumberBox nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.R);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Background = new SolidColorBrush(c);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 0);

                nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.G);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Background = new SolidColorBrush(c);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 1);

                nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.B);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Background = new SolidColorBrush(c);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 2);

                nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.A);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Background = new SolidColorBrush(c);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 3);
            }
            else if (type == typeof(MyEngine.MPoint))
            {
                this.ColumnDefinitions.Add(new ColumnDefinition());
                this.ColumnDefinitions.Add(new ColumnDefinition());
                this.ColumnDefinitions.Add(new ColumnDefinition());

                MyEngine.MPoint point = (MyEngine.MPoint)this.Object;

                NumberBox nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.X);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 0);

                nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.Y);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 1);

                nb = new NumberBox();
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.Z);
                nb.ToolTip = nb.Text;
                nb.Margin = new Thickness(0, 5, 3, 5);
                nb.Changed += new RoutedEventHandler(value_Changed);
                this.Children.Add(nb);
                Grid.SetColumn(nb, 2);
            }
            else if (type.GetInterface("IList") != null)
            {
                RowDefinition rd = new RowDefinition();
                rd.Height = new GridLength(0, GridUnitType.Auto);
                this.RowDefinitions.Add(rd);
                rd = new RowDefinition();
                rd.Height = new GridLength(0, GridUnitType.Auto);
                this.RowDefinitions.Add(rd);

                IList collection = this.Object as IList;

                Label label = new Label();
                label.Content = "(" + collection.Count + " Items)";
                label.FontWeight = FontWeights.Bold;
                label.Foreground = Brushes.SlateGray;
                this.Children.Add(label);
                Grid.SetRow(label, 0);

                StackPanel sp = new StackPanel();
                sp.Orientation = Orientation.Vertical;
                sp.Visibility = System.Windows.Visibility.Collapsed;
                this.Children.Add(sp);
                Grid.SetRow(sp, 1);

                foreach (var item in collection)
                {
                    PropertyGridItem pgi = new PropertyGridItem(item);
                    pgi.CanWrite = this.CanWrite;
                    pgi.Changed += new RoutedEventHandler(value_Changed);
                    sp.Children.Add(pgi);
                }
                this.IsEnabled = true;

                label.PreviewMouseDown += (s, e) =>
                {
                    if (e.LeftButton == System.Windows.Input.MouseButtonState.Pressed)
                    {
                        if (sp.Visibility == System.Windows.Visibility.Collapsed)
                            sp.Visibility = System.Windows.Visibility.Visible;
                        else
                            sp.Visibility = System.Windows.Visibility.Collapsed;
                    }
                };
            }
            else if (type.IsSubclassOf(typeof(System.Drawing.Image)))
            {
                System.Drawing.Bitmap img = this.Object as System.Drawing.Bitmap;

                var bitmapSource = new System.Windows.Media.Imaging.BitmapImage();
                bitmapSource.BeginInit();
                var memoryStream = new System.IO.MemoryStream();
                img.Save(memoryStream, System.Drawing.Imaging.ImageFormat.Bmp);
                memoryStream.Seek(0, System.IO.SeekOrigin.Begin);
                bitmapSource.StreamSource = memoryStream;
                bitmapSource.EndInit();

                Image image = new Image();
                image.Source = bitmapSource;
                image.ToolTip = this.Name;
                this.Children.Add(image);
            }
            else
            {
                TextBox tb = new TextBox();
                tb.Text = this.Object.ToString();
                tb.ToolTip = tb.Text;
                tb.TextChanged += new TextChangedEventHandler(value_Changed);
                this.Children.Add(tb);
            }
        }

        public void Update()
        {
            if (this.Children.Count == 0)
                return;

            Type type = this.Object != null ? this.Object.GetType() : null;
            if (type == null || this.GetList != null)
            {
                TextBox tb = this.Children[0] as TextBox;
                if (type == typeof(object))
                    tb.Text = "None";
                else
                    tb.Text = this.Object.ToString();
                tb.ToolTip = tb.Text;
            }
            else if (type == typeof(uint) || type == typeof(int) ||
                type == typeof(float) || type == typeof(double))
            {
                NumberBox nb = this.Children[0] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                if (type == typeof(uint))
                    nb.SetValueWithoutRaiseChanged((uint)this.Object);
                else if (type == typeof(int))
                    nb.SetValueWithoutRaiseChanged((int)this.Object);
                else
                    nb.SetValueWithoutRaiseChanged((double)this.Object);
                nb.ToolTip = nb.Text;
            }
            else if (type == typeof(bool))
            {
                ComboBox cb = this.Children[0] as ComboBox;
                cb.Text = this.Object.ToString();
                cb.SelectedItem = cb.Text;
                cb.ToolTip = cb.Text;
            }
            else if (type.IsEnum)
            {
                ComboBox cb = this.Children[0] as ComboBox;
                cb.Text = this.Object.ToString();
                cb.SelectedItem = cb.Text;
                cb.ToolTip = cb.Text;
            }
            else if (type == typeof(MyEngine.MColor))
            {
                MyEngine.MColor color = (MyEngine.MColor)this.Object;
                Color c = Color.FromArgb((byte)(color.A * 255), (byte)(color.R * 255), (byte)(color.G * 255), (byte)(color.B * 255));

                NumberBox nb = this.Children[0] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.R);
                nb.ToolTip = nb.Text;

                nb = this.Children[1] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.G);
                nb.ToolTip = nb.Text;

                nb = this.Children[2] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.B);
                nb.ToolTip = nb.Text;

                nb = this.Children[3] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(color.A);
                nb.ToolTip = nb.Text;
            }
            else if (type == typeof(MyEngine.MPoint))
            {
                MyEngine.MPoint point = (MyEngine.MPoint)this.Object;

                NumberBox nb = this.Children[0] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.X);
                nb.ToolTip = nb.Text;

                nb = this.Children[1] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.Y);
                nb.ToolTip = nb.Text;

                nb = this.Children[2] as NumberBox;
                nb.StringFormat = this.FloatStringFormat;
                nb.SetValueWithoutRaiseChanged(point.Z);
                nb.ToolTip = nb.Text;
            }
            else if (type.GetInterface("IList") != null)
            {
                IList collection = this.Object as IList;

                Label label = this.Children[0] as Label;
                label.Content = "(" + collection.Count + " Items)";

                StackPanel sp = this.Children[1] as StackPanel;

                int i = 0;
                foreach (var item in collection)
                {
                    PropertyGridItem pgi = sp.Children[i] as PropertyGridItem;
                    pgi.Object = item;
                    pgi.CanWrite = this.CanWrite;
                    i++;
                }
                this.IsEnabled = true;
            }
            else if (type.IsSubclassOf(typeof(System.Drawing.Image)))
            {
                System.Drawing.Image img = this.Object as System.Drawing.Image;
                System.IO.MemoryStream renderStream = new System.IO.MemoryStream();
                img.Save(renderStream, System.Drawing.Imaging.ImageFormat.Bmp);
                System.Windows.Media.Imaging.BitmapImage bitmapImage = new System.Windows.Media.Imaging.BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = renderStream;
                bitmapImage.EndInit();

                Image image = this.Children[0] as Image;
                image.Source = bitmapImage;
                image.ToolTip = this.Name;
            }
            else
            {
                TextBox tb = this.Children[0] as TextBox;
                tb.Text = this.Object.ToString();
                tb.ToolTip = tb.Text;
            }
        }

        private void value_Changed(object sender, RoutedEventArgs e)
        {
            Type type = this.Object != null ? this.Object.GetType() : null;
            if (type == null || this.GetList != null)
            { }
            else if ((type == typeof(uint)) && (sender is NumberBox))
                SetValue(ObjectProperty, (uint)System.Math.Abs((sender as NumberBox).Value));
            else if ((type == typeof(int)) && (sender is NumberBox))
                SetValue(ObjectProperty, (int)(sender as NumberBox).Value);
            else if ((type == typeof(float) || type == typeof(double)) && (sender is NumberBox))
                SetValue(ObjectProperty, (double)(sender as NumberBox).Value);
            else if ((type == typeof(bool)) && (sender is ComboBox))
                SetValue(ObjectProperty, (bool)((sender as ComboBox).SelectedItem.ToString() == "True"));
            else if ((type.IsEnum) && (sender is ComboBox))
            {
                SetValue(ObjectProperty, Enum.Parse(type, (sender as ComboBox).SelectedItem.ToString()));
            }
            else if (type == typeof(MyEngine.MColor))
            {
                NumberBox vbR = this.Children[0] as NumberBox;
                NumberBox vbG = this.Children[1] as NumberBox;
                NumberBox vbB = this.Children[2] as NumberBox;
                NumberBox vbA = this.Children[3] as NumberBox;

                if (vbR != null && vbG != null && vbB != null && vbA != null)
                {
                    if (sender != null)
                        SetValue(ObjectProperty, new MyEngine.MColor(vbR.Value, vbG.Value, vbB.Value, vbA.Value));

                    Color c = Color.FromArgb((byte)(vbA.Value * 255), (byte)(vbR.Value * 255), (byte)(vbG.Value * 255), (byte)(vbB.Value * 255));
                    vbR.Background = new SolidColorBrush(c);
                    vbG.Background = new SolidColorBrush(c);
                    vbB.Background = new SolidColorBrush(c);
                    vbA.Background = new SolidColorBrush(c);
                }
            }
            else if (type == typeof(MyEngine.MPoint) && sender != null)
            {
                NumberBox vbX = this.Children[0] as NumberBox;
                NumberBox vbY = this.Children[1] as NumberBox;
                NumberBox vbZ = this.Children[2] as NumberBox;

                if (vbX != null && vbY != null && vbZ != null)
                    SetValue(ObjectProperty, new MyEngine.MPoint(vbX.Value, vbY.Value, vbZ.Value));
            }
            else if (type.GetInterface("IList") != null)
            {
                StackPanel sp = this.Children[1] as StackPanel;
                if (sp != null)
                {
                    int i = 0;
                    IList list = (this.Object as IList);
                    foreach (var item in sp.Children)
                    {
                        PropertyGridItem pgi = item as PropertyGridItem;
                        if (pgi != null && list[i].GetType() == pgi.Object.GetType())
                            list[i] = pgi.Object;
                        i++;
                    }
                }
            }
            else if (sender is TextBox)
                SetValue(ObjectProperty, (sender as TextBox).Text);

            this.OnChanged();
        }

        private void button_Click(object sender, RoutedEventArgs e)
        {
            Button btn = sender as Button;
            if (btn != null && this.GetList != null)
            {
                ContextMenu cm = new ContextMenu();
                {
                    MenuItem mi = new MenuItem();
                    mi.Header = "None";
                    mi.Tag = null;
                    mi.Click += new RoutedEventHandler(menuItem_Click);
                    cm.Items.Add(mi);
                }

                List<object> list = this.GetList();
                if (list == null)
                    return;
                foreach (object obj in list)
                {
                    MenuItem mi = new MenuItem();
                    mi.Header = obj.ToString();
                    mi.Tag = obj;
                    mi.Click += new RoutedEventHandler(menuItem_Click);
                    cm.Items.Add(mi);
                }
                cm.Tag = btn.Tag;
                cm.PlacementTarget = btn;
                cm.IsOpen = true;
            }
        }

        private void menuItem_Click(object sender, RoutedEventArgs e)
        {
            MenuItem mi = sender as MenuItem;
            if (mi != null)
            {
                ContextMenu cm = mi.Parent as ContextMenu;
                if (cm != null)
                {
                    TextBox tb = cm.Tag as TextBox;
                    if (tb != null)
                    {
                        if (mi.Tag != null)
                        {
                            this.Object = mi.Tag;
                            tb.Text = mi.Tag.ToString();
                        }
                        else
                        {
                            this.Object = null;
                            tb.Text = "None";
                        }
                    }
                }
            }
        }


    }
}
