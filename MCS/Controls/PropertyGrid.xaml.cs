using System;
using System.Collections.Generic;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;

namespace MCS.Controls
{
    /// <summary>
    /// Interaction logic for PropertyGrid.xaml
    /// </summary>
    public partial class PropertyGrid : UserControl
    {
        public delegate List<object> GetListDelegate(string propertyName);

        public static readonly DependencyProperty ObjectProperty =
            DependencyProperty.Register("Object", typeof(object), typeof(PropertyGrid), new PropertyMetadata(OnPropertyChanged));
        
        public static readonly DependencyProperty GetListProperty =
            DependencyProperty.Register("GetList", typeof(GetListDelegate), typeof(PropertyGrid), new PropertyMetadata(OnPropertyChanged));

        public static readonly DependencyProperty ShowParentPropertiesProperty =
            DependencyProperty.Register("ShowParentProperties", typeof(bool), typeof(PropertyGrid), new PropertyMetadata(OnPropertyChanged));

        public static readonly DependencyProperty ExpandedProperty =
            DependencyProperty.Register("Expanded", typeof(bool), typeof(PropertyGrid), new PropertyMetadata(OnPropertyChanged));

        public static readonly RoutedEvent ChangedEvent =
            EventManager.RegisterRoutedEvent("Changed", RoutingStrategy.Bubble, typeof(RoutedEventHandler), typeof(PropertyGrid));


        public object Object
        {
            get { return GetValue(ObjectProperty); }
            set { SetValue(ObjectProperty, value); }
        }

        public bool ShowParentProperties
        {
            get { return (bool)GetValue(ShowParentPropertiesProperty); }
            set { SetValue(ShowParentPropertiesProperty, value); }
        }

        public bool Expanded
        {
            get { return (bool)GetValue(ExpandedProperty); }
            set { SetValue(ExpandedProperty, value); }
        }
        private List<string> expandedGroups;

        public GetListDelegate GetList
        {
            get { return (GetListDelegate)GetValue(GetListProperty); }
            set { SetValue(GetListProperty, value); }
        }


        public PropertyGrid()
        {
            InitializeComponent();

            this.ShowParentProperties = true;
            this.Expanded = false;
            this.expandedGroups = new List<string>();
        }

        public PropertyGrid(object obj)
            : this()
        {
            this.Object = obj;
        }


        public event RoutedEventHandler Changed
        {
            add { AddHandler(ChangedEvent, value); }
            remove { RemoveHandler(ChangedEvent, value); }
        }

        protected virtual void OnChanged(PropertyGridItem pgi)
        {
            RoutedEventArgs args = new RoutedEventArgs(ChangedEvent, pgi);
            RaiseEvent(args);
        }

        private static void OnPropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs e)
        {
            PropertyGrid pg = source as PropertyGrid;
            if (pg == null)
                return;

            pg.Refresh();
        }

        private void filterTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            this.Refresh();
        }


        public void Refresh()
        {
            this.propertiesGrid.RowDefinitions.Clear();
            this.propertiesGrid.Children.Clear();
            if (this.Object == null)
                return;

            Type type = this.Object.GetType();

            PropertyInfo[] pis = type.GetProperties();
            // sort
            Array.Sort(pis, (a, b) =>
            {

                MyEngine.MPropertyAttribute aAtt = a.GetCustomAttribute(typeof(MyEngine.MPropertyAttribute)) as MyEngine.MPropertyAttribute;
                string aGroup = aAtt != null ? aAtt.Group : string.Empty;
                MyEngine.MPropertyAttribute bAtt = b.GetCustomAttribute(typeof(MyEngine.MPropertyAttribute)) as MyEngine.MPropertyAttribute;
                string bGroup = bAtt != null ? bAtt.Group : string.Empty;
                if (aGroup != bGroup)
                    return aGroup.CompareTo(bGroup);

                return a.Name.CompareTo(b.Name);
            });

            string group = string.Empty;
            foreach (PropertyInfo pi in pis)
            {
                if (!this.ShowParentProperties)
                    if (type.BaseType.GetProperty(pi.Name) != null)
                        continue;

                MyEngine.MPropertyAttribute att = pi.GetCustomAttribute(typeof(MyEngine.MPropertyAttribute)) as MyEngine.MPropertyAttribute;
                if (att == null)
                    continue;

                if (!pi.Name.ToLowerInvariant().Contains(this.filterTextBox.Text.ToLowerInvariant()))
                    continue;

                bool expanded = this.Expanded ^ this.expandedGroups.Contains(att.Group); // xor
                expanded |= !string.IsNullOrEmpty(this.filterTextBox.Text);
                if (string.IsNullOrEmpty(group) || group != att.Group)
                {
                    group = att.Group;
                    this.addGroup(group, expanded);
                }

                if (!expanded)
                    continue;

                object value = pi.GetValue(this.Object, null);
                if (value == null)
                    value = new object();

                this.addPropery(pi.Name, value, pi.CanWrite, att.Description, att.Choosable);
            }
        }

        private void addGroup(string name, bool expanded)
        {
            int row = this.propertiesGrid.RowDefinitions.Count;

            RowDefinition rd = new RowDefinition();
            rd.Height = new GridLength(0, GridUnitType.Auto);
            this.propertiesGrid.RowDefinitions.Add(rd);

            Label nameLabel = new Label();
            if (expanded)
                nameLabel.Content = "- " + name;
            else
                nameLabel.Content = "+ " + name;
            nameLabel.ToolTip = name;
            nameLabel.FontStyle = FontStyles.Italic;
            nameLabel.FontWeight = FontWeights.Bold;
            nameLabel.MouseDown += (s, e) =>
            {
                if (e.LeftButton == System.Windows.Input.MouseButtonState.Pressed)
                {
                    if (this.expandedGroups.Contains(name))
                        this.expandedGroups.Remove(name);
                    else
                        this.expandedGroups.Add(name);
                    this.Refresh();
                }
            };
            this.propertiesGrid.Children.Add(nameLabel);
            Grid.SetColumnSpan(nameLabel, 2);
            Grid.SetRow(nameLabel, row);
        }

        private void addPropery(string name, object value, bool canWrite, string desc, bool choosable)
        {
            int row = this.propertiesGrid.RowDefinitions.Count;

            RowDefinition rd = new RowDefinition();
            rd.Height = new GridLength(0, GridUnitType.Auto);
            this.propertiesGrid.RowDefinitions.Add(rd);

            Label nameLabel = new Label();
            nameLabel.Content = name;
            if (!string.IsNullOrEmpty(desc))
                nameLabel.ToolTip = desc;
            nameLabel.Margin = new Thickness(3);
            this.propertiesGrid.Children.Add(nameLabel);
            Grid.SetColumn(nameLabel, 0);
            Grid.SetRow(nameLabel, row);

            PropertyGridItem pgi = new PropertyGridItem(value);
            pgi.Name = name;
            pgi.CanWrite = canWrite;
            if (this.GetList != null && choosable)
                pgi.GetList = () => { return this.GetList(pgi.Name); };
            pgi.Changed += new RoutedEventHandler(property_Changed);
            this.propertiesGrid.Children.Add(pgi);
            Grid.SetColumn(pgi, 1);
            Grid.SetRow(pgi, row);
        }

        private void property_Changed(object sender, RoutedEventArgs e)
        {
            PropertyGridItem pgi = sender as PropertyGridItem;
            if (pgi != null)
            {
                PropertyInfo pi = this.Object.GetType().GetProperty(pgi.Name);
                if (pgi.Object.GetType() == typeof(object))
                    pi.SetValue(this.Object, null, null);
                else
                    pi.SetValue(this.Object, pgi.Object, null);
                this.OnChanged(pgi);
            }
        }
    }
}
