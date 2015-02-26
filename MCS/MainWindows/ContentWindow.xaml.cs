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
        public MContentManager ContentManager { get; private set; }

        public static List<uint> SelectedElements = new List<uint>();


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

        public Dictionary<string, TreeItem> PathsTree
        {
            get
            {
                Dictionary<string, TreeItem> result = new Dictionary<string, TreeItem>();

                List<string> paths = this.ContentManager.Paths;
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

                List<MContentElement> elements = this.ContentManager.Content;
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
                if (ContentWindow.SelectedElements.Count > 0)
                    return this.ContentManager.GetElement(ContentWindow.SelectedElements[0], true);

                return null;
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
                        if (!this.ContentManager.CreatePath(packageName + "#"))
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
                    if (this.SelectedTreeItem == null)
                        return;

                    string folderName = TextDialogBox.Show("Create Folder", "Name");
                    if (!string.IsNullOrEmpty(folderName))
                    {
                        if (!this.ContentManager.CreatePath(this.SelectedTreeItem.Value.FullPath + folderName + "\\"))
                            ExtendedMessageBox.Show("Cannot create the folder '" + folderName + "'!", "Create folder", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand RenamePathCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (this.SelectedTreeItem == null)
                        return;

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

                        if (!this.ContentManager.RenamePath(oldPath, newPath))
                            ExtendedMessageBox.Show("Cannot rename path '" + folderName + "'!", "Rename path", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand DeletePathCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (this.SelectedTreeItem == null)
                        return;

                    string path = this.SelectedTreeItem.Value.FullPath;
                    ExtendedMessageBoxResult res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + path + "'?", "Delete path", ExtendedMessageBoxButton.YesNo, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes)
                    {
                        if (!this.ContentManager.DeletePath(path))
                            ExtendedMessageBox.Show("Cannot delete path '" + path + "'!", "Delete path", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }


        // Content Element commands
        public ICommand CloneElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = this.ContentManager.GetElement(ContentWindow.SelectedElements[0], false);
                    string newName = TextDialogBox.Show("Clone", "Name", elem.Name + "2");
                    if (!string.IsNullOrEmpty(newName))
                    {
                        if (this.ContentManager.CloneElement(elem.ID, newName) == null)
                            ExtendedMessageBox.Show("Cannot clone content element '" + elem.Name + "' to '" + newName + "'!", "Clone element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand RenameElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = this.ContentManager.GetElement(ContentWindow.SelectedElements[0], false);
                    string newName = TextDialogBox.Show("Rename", "Name", elem.Name);
                    if (!string.IsNullOrEmpty(newName) && elem.Name != newName)
                    {
                        if (!this.ContentManager.RenameElement(elem.ID, newName))
                            ExtendedMessageBox.Show("Cannot rename content element '" + elem.Name + "' to '" + newName + "'!", "Rename element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand MoveElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = this.ContentManager.GetElement(ContentWindow.SelectedElements[0], false);
                    string newFullPath = TextDialogBox.Show("Move", "Name", elem.FullPath);
                    if (!string.IsNullOrEmpty(newFullPath) && elem.FullPath != newFullPath)
                    {
                        if (!this.ContentManager.MoveElement(elem.ID, newFullPath))
                            ExtendedMessageBox.Show("Cannot move content element '" + elem.Name + "' to '" + newFullPath + "'!", "Move element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                    }
                });
            }
        }

        public ICommand DeleteElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count == 0)
                        return;

                    ExtendedMessageBoxResult res = ExtendedMessageBoxResult.None;
                    ExtendedMessageBoxButton button = ContentWindow.SelectedElements.Count > 1 ? ExtendedMessageBoxButton.YesYesToAllNoNoToAll : ExtendedMessageBoxButton.YesNo;
                    List<uint> selectedElements = new List<uint>(ContentWindow.SelectedElements);
                    foreach (var id in selectedElements)
                    {
                        MContentElement elem = this.ContentManager.GetElement(id, false);
                        if (res != ExtendedMessageBoxResult.YesToAll && res != ExtendedMessageBoxResult.NoToAll)
                            res = ExtendedMessageBox.Show("Are you sure that you want to delete '" + elem.Name + "'?", "Delete element", button, ExtendedMessageBoxImage.Question);
                        if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                        {
                            if (!this.ContentManager.DeleteElement(elem.ID))
                                ExtendedMessageBox.Show("Cannot delete content element '" + elem.Name + "'!", "Delete element", ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.Error);
                        }
                    }

                    if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                    {
                        ContentWindow.SelectedElements.Clear();
                    }
                });
            }
        }


        public ICommand ImportElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (this.SelectedTreeItem == null)
                        return;

                    OpenFileDialog openFileDialog = new OpenFileDialog();
                    string filter = "All Compatible Files (*.obj, *.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr, *.wav)|*.obj;*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr;*.wav|";
                    filter += "Object Files (*.obj)|*.obj|";
                    filter += "Picture Files (*.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr)|*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr|";
                    filter += "WAVE Files (*.wav)|*.wav|";
                    filter += "All Files (*.*)|*.*";
                    openFileDialog.Filter = filter;
                    openFileDialog.Multiselect = true;

                    if (openFileDialog.ShowDialog() == true)
                        this.import(new List<string>(openFileDialog.FileNames));
                });
            }
        }

        public ICommand ExportElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = this.ContentManager.GetElement(ContentWindow.SelectedElements[0], false);

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
                    /* TODO: add other content elements
                    else if (elem.Type == EContentElementType.Material)
                    {
                        saveFileDialog.Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "xml";
                    }
                    else if (elem.Type == EContentElementType.Texture)
                    {
                        saveFileDialog.Filter = "Picture Files (*.bmp, *.jpg, *.gif, *.png, *.tiff, *.hdr)|*.bmp;*.jpg;*.gif;*.png;*.tiff;*.hdr|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "png";
                    }
                    else if (elem.Type == EContentElementType.Sound)
                    {
                        saveFileDialog.Filter = "WAVE Files (*.wav)|*.wav|All Files (*.*)|*.*";
                        saveFileDialog.DefaultExt = "wav";
                    } // */

                    if (saveFileDialog.ShowDialog() == true)
                        this.export(saveFileDialog.FileName);

                });
            }
        }

        public ICommand ExportToPackageElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count == 0)
                        return;

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

                        foreach(uint selection in ContentWindow.SelectedElements)
                            this.ContentManager.ExportToPackage(saveFileDialog.FileName, selection);
                    }

                });
            }
        }


        // Add Content Element commands

        #endregion


        public ContentWindow(MContentManager contentManager)
        {
            InitializeComponent();
            this.DataContext = this;

            this.ContentManager = contentManager;
            this.ContentManager.Changed += (s, e) =>
            {
                this.OnPropertyChanged("PathsTree");
                this.OnPropertyChanged("Contents");
            };

            this.filter = string.Empty;
            this.selectedTreeItem = null;
        }


        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            KeyValuePair<string, TreeItem> selectedItem = (KeyValuePair<string, TreeItem>)e.NewValue;
            this.SelectedTreeItem = selectedItem.Value;
        }

        private void ListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            foreach (ContentItem item in e.RemovedItems)
                ContentWindow.SelectedElements.Remove(item.ID);

            foreach (ContentItem item in e.AddedItems)
                if (!ContentWindow.SelectedElements.Contains(item.ID))
                    ContentWindow.SelectedElements.Add(item.ID);

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
                /* TODO: add other content elements
                else if (textureExts.Contains(ext))
                    type = EContentElementType.Texture;
                else if (ext == ".wave")
                    type = EContentElementType.Sound; // */
                else
                    continue;

                string name = Path.GetFileNameWithoutExtension(filenames[i]);
                string relativePath = Path.GetDirectoryName(filenames[i]).Remove(0, rootDir.Length).Trim('\\');
                if (relativePath != string.Empty) relativePath += "\\";
                string fullPath = selectedTreeItem.FullPath + relativePath;

                if (!this.ContentManager.ContainPath(fullPath))
                    this.ContentManager.CreatePath(fullPath);

                // if allready exists
                MContentElement elem = this.ContentManager.GetElement(fullPath + name, true);
                uint id = 0;
                if (elem != null)
                {
                    id = elem.ID;
                    if (res != ExtendedMessageBoxResult.YesToAll && res != ExtendedMessageBoxResult.NoToAll)
                        res = ExtendedMessageBox.Show("Element '" + name + "' already exists in Content! \nDo you want to replace it?", "Confirm", button, ExtendedMessageBoxImage.Question);
                    if (res == ExtendedMessageBoxResult.Yes || res == ExtendedMessageBoxResult.YesToAll)
                        this.ContentManager.DeleteElement(id);
                    else
                        continue;
                }

                // add element
                string package = MContentManager.GetPackage(fullPath);
                string path = MContentManager.GetPath(fullPath);
                elem = this.ContentManager.AddElement(type, name, package, path, id);

                if (elem != null)
                {
                    if (type == EContentElementType.Mesh)
                        ((MMesh)elem).LoadFromOBJFile(filenames[i]);
                    /* TODO: add other content elements
                    else if (type == EContentElementType.Texture)
                        ((MTexture)elem).LoadFromFile(fileDrop);
                    else if (type == EContentElementType.Sound)
                        ((MSound)elem).LoadFromWAVEFile(fileDrop); // */
                    this.ContentManager.SaveElement(elem.ID);
                }
            }
        }

        private void export(string filename)
        {
            MContentElement elem = this.ContentManager.GetElement(ContentWindow.SelectedElements[0]);

            if (elem.Type == EContentElementType.Mesh)
                ((MMesh)elem).SaveToOBJFile(filename);
            /* TODO: add other content elements
            else if (elem.Type == EContentElementType.Material)
                ((MMaterial)elem).SaveToXMLFile(filename);
            else if (elem.Type == EContentElementType.Texture)
                ((MTexture)elem).SaveToFile(filename);
            else if (elem.Type == EContentElementType.Sound)
                ((MSound)elem).SaveToWAVEFile(filename); // */
        }

        
        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            if (this.PropertyChanged != null)
			    this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }

    }
}
