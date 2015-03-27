using MCS.Dialogs;
using MCS.Managers;
using MyEngine;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for LayersWindow.xaml
    /// </summary>
    public partial class LayersWindow : Window
    {
        public struct LayerItem
        {
            public string Name { get; set; }
            public ObservableCollection<MSceneElement> Elements { get; set; }
            public bool? Visible
            {
                get
                {
                    bool? result = true;
                    foreach (var mse in this.Elements)
                    {
                        if (result == true && !mse.Visible)
                            result = false;
                        else if (result == false && mse.Visible)
                        {
                            result = null;
                            break;
                        }
                    }

                    return result;
                }
                set
                {
                    if (value == null)
                        return;

                    foreach (var mse in this.Elements)
                        mse.Visible = value.Value;
                }
            }

            public LayerItem(string name) : this()
            {
                this.Name = name;
                this.Elements = new ObservableCollection<MSceneElement>();
            }
        }


        private MSceneManager sceneManager;


        public ObservableCollection<LayerItem> Items { get; private set; }

        public Object SelectedItem { get; set; }

        #region Commands

        public ICommand CreateLayerCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string layerName = TextDialogBox.Show("Create Layer", "Name");
                    if (!string.IsNullOrEmpty(layerName))
                    {
                        if (!this.sceneManager.CreateLayer(layerName))
                            ExtendedMessageBox.Show("Cannot create the layer '" + layerName + "'!", "Create Layer", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand RenameLayerCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (!(this.SelectedItem is LayerItem))
                        return;

                    LayerItem layerItem = (LayerItem)this.SelectedItem;
                    string layerName = TextDialogBox.Show("Rename Layer", "Name", layerItem.Name);
                    if (!string.IsNullOrEmpty(layerName))
                    {
                        if (!this.sceneManager.RenameLayer(layerItem.Name, layerName))
                            ExtendedMessageBox.Show("Cannot rename layer '" + layerItem.Name + "'!", "Rename Layer", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand DeleteLayerCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (!(this.SelectedItem is LayerItem))
                        return;

                    LayerItem layerItem = (LayerItem)this.SelectedItem;
                    ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + layerItem.Name + "'?", "Delete Layer", ExtendedMessageBoxButton.YesNo, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes)
                    {
                        if (!this.sceneManager.DeleteLayer(layerItem.Name))
                            ExtendedMessageBox.Show("Cannot delete layer '" + layerItem.Name + "'!", "Delete Layer", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        #endregion


        public LayersWindow(MSceneManager sceneManager)
        {
            InitializeComponent();

            if (sceneManager == null)
                throw new ArgumentNullException("sceneManager");

            this.Items = new ObservableCollection<LayerItem>();
            this.DataContext = this;
            this.sceneManager = sceneManager;

            this.sceneManager.Changed += sceneManager_Changed;
            sceneManager_Changed(null, null);
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            this.sceneManager.Changed -= sceneManager_Changed;
        }

        private void sceneManager_Changed(MSceneManager sender, MSceneElement element)
        {
            this.Items.Clear();
            List<string> layers = this.sceneManager.Layers;
            foreach (var layer in layers)
                this.Items.Add(new LayerItem(layer));

            List<MSceneElement> mses = this.sceneManager.Elements;
            foreach (var mse in mses)
            {
                foreach (var item in this.Items)
                {
                    if (mse.Layer == item.Name)
                    {
                        item.Elements.Add(mse);
                        break;
                    }
                }
            }
        }

        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            this.SelectedItem = e.NewValue;

            MSceneElement mse = this.SelectedItem as MSceneElement;
            if (mse != null)
            {
                MSelector.Clear(MSelector.ESelectionType.SceneElement);
                MSelector.Select(MSelector.ESelectionType.SceneElement, mse.ID);
            }
        }

        private void TreeView_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                MSceneElement mse = this.SelectedItem as MSceneElement;
                if (mse == null)
                    return;

                DragDropEffects effect = DragDropEffects.Move;
                DragDrop.DoDragDrop(sender as DependencyObject, mse, effect);
            }
        }

        private void LayerCheckBox_Drop(object sender, DragEventArgs e)
        {
            MSceneElement mse = e.Data.GetData(typeof(MSceneElement)) as MSceneElement;

            CheckBox checkBox = sender as CheckBox;
            if (mse != null && checkBox != null)
                this.sceneManager.SetElementLayer(mse.ID, checkBox.Content.ToString());
        }

    }
}
