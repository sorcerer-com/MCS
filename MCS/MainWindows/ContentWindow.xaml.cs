﻿using MCS.Dialogs;
using MCS.Managers;
using Microsoft.Win32;
using MyEngine;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for ContentWindow.xaml
    /// </summary>
    public partial class ContentWindow : Window, INotifyPropertyChanged
    {
        #region Helpers Structures

        public struct TreeItem
        {
            public string Name { get; private set; }
            public string FullPath { get; private set; }
            public string Image { get; private set; }
            public Dictionary<string, TreeItem> Children { get; private set; }

            public TreeItem(string name, string fullPath, string image)
                : this()
            {
                this.Name = name;
                this.FullPath = fullPath;
                this.Image = image;
                this.Children = new Dictionary<string, TreeItem>();
            }
        }

        public struct ContentItem
        {
            public string Image { get; private set; }
            public uint ID { get; private set; }
            public string Name { get; private set; }
            public string Info { get; private set; }

            public ContentItem(string image, uint id, string name, string info)
                : this()
            {
                this.Image = image;
                this.ID = id;
                this.Name = name;
                this.Info = info;
            }
        }

        #endregion


        private MContentManager contentManager;

        private List<uint> changedElements;


        public Dictionary<string, TreeItem> PathsTree
        {
            get
            {
                Dictionary<string, TreeItem> result = new Dictionary<string, TreeItem>();

                List<string> paths = this.contentManager.Paths;
                foreach(string fullPath in paths)
                {
                    string package = MContentManager.GetPackage(fullPath);
                    string path = MContentManager.GetPath(fullPath);

                    if (!result.ContainsKey(package))
                        result.Add(package, new TreeItem(package, package + "#", "/Images/ContentWindow/package.png"));
                    
                    TreeItem curr = result[package];
                    string[] folders = path.Split(new char[] { '\\' }, System.StringSplitOptions.RemoveEmptyEntries);
                    foreach(string folder in folders)
                    {
                        if (!curr.Children.ContainsKey(folder))
                            curr.Children.Add(folder, new TreeItem(folder, curr.FullPath + folder + "\\", "/Images/ContentWindow/folder.png"));
                        curr = curr.Children[folder];
                    }
                }

                return result;
            }
        }

        public ObservableCollection<ContentItem> Contents
        {
            get
            {
                List<ContentItem> result =
                    new List<ContentItem>();

                List<MContentElement> elements = this.contentManager.Content;
                foreach(MContentElement element in elements)
                {
                    if (!element.FullName.ToLowerInvariant().Contains(this.Filter) &&
                        !element.ID.ToString().ToLowerInvariant().Contains(this.Filter) &&
                        !element.Type.ToString().ToLowerInvariant().Contains(this.Filter))
                        continue;

                    if (this.SelectedTreeItem != null && !element.FullPath.Contains(this.SelectedTreeItem.Value.FullPath))
                        continue;

                    string image = string.Empty;
				    if (element.Type == EContentElementType.Mesh)
					    image = "/Images/ContentWindow/Mesh.png";
				    else if (element.Type == EContentElementType.Material)
					    image = "/Images/ContentWindow/Material.png";
				    else if (element.Type == EContentElementType.Texture)
					    image = "/Images/ContentWindow/Texture.png";
				    else if (element.Type == EContentElementType.UIScreen)
					    image = "/Images/ContentWindow/UIScreen.png";
				    else if (element.Type == EContentElementType.Skeleton)
					    image = "/Images/ContentWindow/Skeleton.png";
				    else if (element.Type == EContentElementType.Sound)
					    image = "/Images/ContentWindow/Sound.png";

                    result.Add(new ContentItem(image, element.ID, element.Name, element.Info));
                }

                result.Sort((item1, item2) => item1.Name.CompareTo(item2.Name));
                return new ObservableCollection<ContentItem>(result);
            }
        }

        private string filter;
        public string Filter
        {
            get { return this.filter; }
            set
            {
                this.filter = value.ToLowerInvariant();
                this.OnPropertyChanged("Filter");
                this.OnPropertyChanged("Contents");
            }
        }

        private TreeItem? selectedTreeItem;
        public TreeItem? SelectedTreeItem
        {
            get { return this.selectedTreeItem; }
            set
            {
                this.selectedTreeItem = value;
                this.OnPropertyChanged("SelectedTreeItem");
                this.OnPropertyChanged("Contents");
            }
        }

        public MContentElement SelectedElement
        {
            get 
            {
                var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                if (selectedElements.Count > 0)
                    return this.contentManager.GetElement(selectedElements[0], true);

                return null;
            }
        }

        public MCS.Controls.PropertyGrid.GetListDelegate GetSelectedContentElementsList
        {
            get
            {
                return (s) =>
                {
                    List<object> res = new List<object>();
                    var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                    foreach (var id in selectedElements)
                        res.Add(this.contentManager.GetElement(id));
                    return res;
                };
            }
        }

        #region Commands

        public ICommand ClearFilterCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                    {
                        this.SelectedTreeItem = null;
                        this.Filter = string.Empty;
                    });
            }
        }


        // Path commands
        public ICommand CreatePackageCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string packageName = TextDialogBox.Show("Create Package", "Name");
                    if (!string.IsNullOrEmpty(packageName))
                    {
                        if (!this.contentManager.CreatePath(packageName + "#"))
                            ExtendedMessageBox.Show("Cannot create the package '" + packageName + "'!", "Create package", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand CreateFolderCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string folderName = TextDialogBox.Show("Create Folder", "Name");
                    if (!string.IsNullOrEmpty(folderName))
                    {
                        if (!this.contentManager.CreatePath(this.SelectedTreeItem.Value.FullPath + folderName + "\\"))
                            ExtendedMessageBox.Show("Cannot create the folder '" + folderName + "'!", "Create folder", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedTreeItem != null; });
            }
        }

        public ICommand RenamePathCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string folderName = TextDialogBox.Show("Rename", "Name", this.SelectedTreeItem.Value.Name);
                    if (!string.IsNullOrEmpty(folderName))
                    {
                        string oldPath = this.SelectedTreeItem.Value.FullPath;
                        string newPath = oldPath.Substring(0, oldPath.Length - this.SelectedTreeItem.Value.Name.Length - 1) + folderName;
                        if (oldPath.EndsWith("#"))
                            newPath += "#";
                        else
                            newPath += "\\";

                        if (oldPath == newPath)
                            return;

                        if (!this.contentManager.RenamePath(oldPath, newPath))
                            ExtendedMessageBox.Show("Cannot rename path '" + folderName + "'!", "Rename path", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedTreeItem != null; });
            }
        }

        public ICommand DeletePathCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string path = this.SelectedTreeItem.Value.FullPath;
                    ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + path + "'?", "Delete path", ExtendedMessageBoxButton.YesNo, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes)
                    {
                        if (!this.contentManager.DeletePath(path))
                            ExtendedMessageBox.Show("Cannot delete path '" + path + "'!", "Delete path", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedTreeItem != null; });
            }
        }


        // Content Element commands
        public ICommand CloneElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MContentElement elem = this.SelectedElement;
                    string newName = TextDialogBox.Show("Clone", "Name", elem.Name + "2");
                    if (!string.IsNullOrEmpty(newName))
                    {
                        if (this.contentManager.CloneElement(elem.ID, newName) == null)
                            ExtendedMessageBox.Show("Cannot clone content element '" + elem.Name + "' to '" + newName + "'!", "Clone element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedElement != null; });
            }
        }

        public ICommand RenameElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MContentElement elem = this.SelectedElement;
                    string newName = TextDialogBox.Show("Rename", "Name", elem.Name);
                    if (!string.IsNullOrEmpty(newName) && elem.Name != newName)
                    {
                        if (!this.contentManager.RenameElement(elem.ID, newName))
                            ExtendedMessageBox.Show("Cannot rename content element '" + elem.Name + "' to '" + newName + "'!", "Rename element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedElement != null; });
            }
        }

        public ICommand MoveElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MContentElement elem = this.SelectedElement;
                    string newFullPath = TextDialogBox.Show("Move", "Name", elem.FullPath);
                    if (!string.IsNullOrEmpty(newFullPath) && elem.FullPath != newFullPath)
                    {
                        if (!this.contentManager.MoveElement(elem.ID, newFullPath))
                            ExtendedMessageBox.Show("Cannot move content element '" + elem.Name + "' to '" + newFullPath + "'!", "Move element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedElement != null; });
            }
        }

        public ICommand DeleteElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                    ExtendedMessageBoxResult res = ExtendedMessageBoxResult.None;
                    ExtendedMessageBoxButton button = selectedElements.Count > 1 ? ExtendedMessageBoxButton.YesYesToAllNoNoToAll : ExtendedMessageBoxButton.YesNo;
                    foreach (var id in selectedElements)
                    {
                        MContentElement elem = this.contentManager.GetElement(id, false);
                        if (res != ExtendedMessageBoxResult.YesToAll && res != ExtendedMessageBoxResult.NoToAll)
                            res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + elem.Name + "'?", "Delete element", button, ExtendedMessageBoxImage.Question);
                        if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                        {
                            if (!this.contentManager.DeleteElement(elem.ID))
                                ExtendedMessageBox.Show("Cannot delete content element '" + elem.Name + "'!", "Delete element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        }
                    }

                    if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                    {
                        MSelector.Clear(MSelector.ESelectionType.ContentElement);
                    }
                }, (o) => MSelector.Count(MSelector.ESelectionType.ContentElement) != 0);
            }
        }


        public ICommand ImportElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    OpenFileDialog openFileDialog = new OpenFileDialog();
                    string filter = "All Compatible Files (*.obj, *.xml, *.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr, *.wav)|*.obj;*.xml;*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr;*.wav|";
                    filter += "Object Files (*.obj)|*.obj|";
                    filter += "Material XML Files (*.xml)|*.xml|";
                    filter += "Picture Files (*.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr)|*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr|";
                    filter += "WAVE Files (*.wav)|*.wav|";
                    filter += "All Files (*.*)|*.*";
                    openFileDialog.Filter = filter;
                    openFileDialog.Multiselect = true;

                    if (openFileDialog.ShowDialog() == true)
                        this.import(new List<string>(openFileDialog.FileNames));
                }, (o) => { return this.SelectedTreeItem != null; });
            }
        }

        public ICommand ExportElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MContentElement elem = this.SelectedElement;

                    SaveFileDialog saveFileDialog = new SaveFileDialog();
                    saveFileDialog.FileName = elem.Name;
                    saveFileDialog.RestoreDirectory = true;
                    saveFileDialog.OverwritePrompt = true;
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

                    if (elem.Type == EContentElementType.Mesh)
                    {
                        saveFileDialog.Filter = "Object Files (*.obj)|*.obj|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "obj";
                    }
                    else if (elem.Type == EContentElementType.Material)
                    {
                        saveFileDialog.Filter = "Material XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "xml";
                    }
                    else if (elem.Type == EContentElementType.Texture)
                    {
                        saveFileDialog.Filter = "Picture Files (*.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr)|*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "png";
                    }
                    /* TODO: add other content elements */

                    if (saveFileDialog.ShowDialog() == true)
                        this.export(saveFileDialog.FileName);

                }, (o) => { return this.SelectedElement != null; });
            }
        }

        public ICommand ExportToPackageElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    SaveFileDialog saveFileDialog = new SaveFileDialog();
                    saveFileDialog.FileName = "package.mpk";
                    saveFileDialog.DefaultExt = "mpk";
                    saveFileDialog.RestoreDirectory = true;
                    saveFileDialog.OverwritePrompt = true;
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

                    if (saveFileDialog.ShowDialog() == true)
                    {
                        if (File.Exists(saveFileDialog.FileName))
                            File.Delete(saveFileDialog.FileName);

                        var selectedElements = MSelector.Elements(MSelector.ESelectionType.ContentElement);
                        foreach (uint selection in selectedElements)
                            this.contentManager.ExportToPackage(saveFileDialog.FileName, selection);
                    }

                }, (o) => MSelector.Count(MSelector.ESelectionType.ContentElement) != 0);
            }
        }


        // Add Content Element commands
        public ICommand AddElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    string name = TextDialogBox.Show("Add " + o.ToString(), "Name");
                    if (!string.IsNullOrEmpty(name))
                    {
                        EContentElementType type;
                        if (o.ToString() == "Material")
                            type = EContentElementType.Material;
                        /* TODO: add other content elements */
                        else
                            return;

                        string package = MContentManager.GetPackage(this.SelectedTreeItem.Value.FullPath);
                        string path = MContentManager.GetPath(this.SelectedTreeItem.Value.FullPath);
                        MContentElement elem = this.contentManager.AddElement(type, name, package, path, 0);
                        if (elem == null)
                            ExtendedMessageBox.Show("Cannot add content element '" + name + "' at '" + this.SelectedTreeItem.Value.FullPath + "'", type.ToString(), ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                }, (o) => { return this.SelectedTreeItem != null; });
            }
        }

        #endregion


        public ContentWindow(MContentManager contentManager)
        {
            InitializeComponent();
            this.DataContext = this;

            this.contentManager = contentManager;
            this.contentManager.Changed += contentManager_Changed;
            this.changedElements = new List<uint>();

            this.filter = string.Empty;
            this.selectedTreeItem = null;

            MSelector.Clear(MSelector.ESelectionType.ContentElement);
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            foreach(var elemID in this.changedElements)
                this.contentManager.SaveElement(elemID);

            this.contentManager.Changed -= contentManager_Changed;
        }

        private void contentManager_Changed(MContentManager sender, MContentElement element)
        {
            if (element == null)
            {
                this.OnPropertyChanged("PathsTree");
                this.OnPropertyChanged("Contents");
            }
        }


        private void contextMenuOpening(object sender, ContextMenuEventArgs e)
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

        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            KeyValuePair<string, TreeItem> selectedItem = (KeyValuePair<string, TreeItem>)e.NewValue;
            this.SelectedTreeItem = selectedItem.Value;
        }

        private void ListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            foreach (ContentItem item in e.RemovedItems)
                MSelector.Deselect(MSelector.ESelectionType.ContentElement, item.ID);

            foreach (ContentItem item in e.AddedItems)
            {
                MSelector.Select(MSelector.ESelectionType.ContentElement, item.ID);
                // to save all selected elements
                //this.changedElements.Add(item.ID);
            }

            this.OnPropertyChanged("SelectedElement");
        }

        private void ListView_Drop(object sender, DragEventArgs e)
        {
            string[] fileDrops = e.Data.GetData(DataFormats.FileDrop) as string[];
            if (fileDrops != null && this.SelectedTreeItem != null)
                this.import(new List<string>(fileDrops));
        }

        private void import(List<string> filenames)
        {
            List<string> textureExts = new List<string> { ".bmp", ".jpg", ".gif", ".png", ".tiff", ".hdr" };

            ExtendedMessageBoxResult res = ExtendedMessageBoxResult.None;
            ExtendedMessageBoxButton button = filenames.Count > 1 ? ExtendedMessageBoxButton.YesYesToAllNoNoToAll : ExtendedMessageBoxButton.YesNo;
            
            string rootDir = filenames.Count > 0 ? Path.GetDirectoryName(filenames[0]) : string.Empty;
            TreeItem selectedTreeItem = this.SelectedTreeItem.Value;

            for (int i = 0; i < filenames.Count; i++)
            {
                // if we drop a folder
                if (Directory.Exists(filenames[i]))
                {
                    filenames.AddRange(Directory.GetDirectories(filenames[i]));
                    filenames.AddRange(Directory.GetFiles(filenames[i]));
                    continue;
                }

                if (!File.Exists(filenames[i]))
                    continue;

                EContentElementType type;
                string ext = Path.GetExtension(filenames[i]).ToLowerInvariant();
                if (ext == ".obj")
                    type = EContentElementType.Mesh;
                else if (ext == ".xml")
                    type = EContentElementType.Material;
                else if (textureExts.Contains(ext))
                    type = EContentElementType.Texture;
                /* TODO: add other content elements */
                else
                    continue;

                string name = Path.GetFileNameWithoutExtension(filenames[i]);
                string relativePath = Path.GetDirectoryName(filenames[i]).Remove(0, rootDir.Length).Trim('\\');
                if (relativePath != string.Empty) relativePath += "\\";
                string fullPath = selectedTreeItem.FullPath + relativePath;

                if (!this.contentManager.ContainsPath(fullPath))
                    this.contentManager.CreatePath(fullPath);

                // if already exists
                MContentElement elem = this.contentManager.GetElement(fullPath + name, true);
                uint id = 0;
                if (elem != null)
                {
                    id = elem.ID;
                    if (res != ExtendedMessageBoxResult.YesToAll && res != ExtendedMessageBoxResult.NoToAll)
                        res = ExtendedMessageBox.Show("Element '" + name + "' already exists in Content! \nDo you want to replace it?", "Confirm", button, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                        this.contentManager.DeleteElement(id);
                    else
                        continue;
                }

                // add element
                string package = MContentManager.GetPackage(fullPath);
                string path = MContentManager.GetPath(fullPath);
                elem = this.contentManager.AddElement(type, name, package, path, id);

                if (elem != null)
                {
                    elem.LoadFromFile(filenames[i]);
                    if (!this.changedElements.Contains(elem.ID))
                        this.changedElements.Add(elem.ID);
                }
            }
        }

        private void export(string filename)
        {
            if (this.SelectedElement != null)
                this.SelectedElement.SaveToFile(filename);
        }


        private void PropertyGrid_Changed(object sender, RoutedEventArgs e)
        {
            MCS.Controls.PropertyGrid pg = sender as MCS.Controls.PropertyGrid;
            if (pg != null && pg.Object is MContentElement)
            {
                MContentElement elem = pg.Object as MContentElement;
                if (!this.changedElements.Contains(elem.ID))
                    this.changedElements.Add(elem.ID);
            }
        }

        
        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            if (this.PropertyChanged != null)
			    this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }

    }
}
