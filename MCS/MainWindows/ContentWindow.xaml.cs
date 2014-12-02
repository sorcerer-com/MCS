using MCS.Dialogs;
using MCS.Managers;
using MEngine;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
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

        public static List<MContentElement> SelectedElements = new List<MContentElement>();


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
            public MContentElement Element { get; private set; }

            public ContentItem(string image, MContentElement element)
                : this()
            {
                this.Image = image;
                this.Element = element;
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
                    string package = MContentElement.GetPackage(fullPath);
                    string path = MContentElement.GetPath(fullPath);

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

                    if (this.SelectedTreeItem != null && !element.FullPath.Equals(this.SelectedTreeItem.Value.FullPath))
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

                    result.Add(new ContentItem(image, element));
                }

                result.Sort((item1, item2) => item1.Element.Name.CompareTo(item2.Element.Name));
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
                            MessageBox.Show("Cannot create the package '" + packageName + "'!", "Create package", MessageBoxButton.OK, MessageBoxImage.Error);
                        
                        this.OnPropertyChanged("PathsTree");
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
                            MessageBox.Show("Cannot create the folder '" + folderName + "'!", "Create folder", MessageBoxButton.OK, MessageBoxImage.Error);
                        
                        this.OnPropertyChanged("PathsTree");
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
                            MessageBox.Show("Cannot rename path '" + folderName + "'!", "Rename path", MessageBoxButton.OK, MessageBoxImage.Error);

                        this.OnPropertyChanged("PathsTree");
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
                    MessageBoxResult res = MessageBox.Show("Are you sure that you want to delete '" + path + "'?", "Delete path", MessageBoxButton.YesNo, MessageBoxImage.Question);
                    if (res == MessageBoxResult.Yes)
                    {
                        if (!this.ContentManager.DeletePath(path))
                            MessageBox.Show("Cannot delete path '" + path + "'!", "Delete path", MessageBoxButton.OK, MessageBoxImage.Error);

                        this.OnPropertyChanged("PathsTree");
                        this.OnPropertyChanged("Contents");
                    }
                });
            }
        }


        // Content element commands
        public ICommand CloneElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = ContentWindow.SelectedElements[0];
                    string newName = TextDialogBox.Show("Clone", "Name", elem.Name + "2");
                    if (!string.IsNullOrEmpty(newName))
                    {
                        if (this.ContentManager.CloneElement(elem.ID, newName) == null)
                            MessageBox.Show("Cannot clone content element '" + elem.Name + "' to '" + newName + "'!", "Clone element", MessageBoxButton.OK, MessageBoxImage.Error);

                        this.OnPropertyChanged("Contents");
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

                    MContentElement elem = ContentWindow.SelectedElements[0];
                    string newName = TextDialogBox.Show("Rename", "Name", elem.Name);
                    if (!string.IsNullOrEmpty(newName) && elem.Name != newName)
                    {
                        if (!this.ContentManager.RenameElement(elem.ID, newName))
                            MessageBox.Show("Cannot rename content element '" + elem.Name + "' to '" + newName + "'!", "Rename element", MessageBoxButton.OK, MessageBoxImage.Error);

                        this.OnPropertyChanged("Contents");
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

                    MContentElement elem = ContentWindow.SelectedElements[0];
                    string newFullPath = TextDialogBox.Show("Move", "Name", elem.FullPath);
                    if (!string.IsNullOrEmpty(newFullPath) && elem.FullPath != newFullPath)
                    {
                        if (!this.ContentManager.MoveElement(elem.ID, newFullPath))
                            MessageBox.Show("Cannot move content element '" + elem.Name + "' to '" + newFullPath + "'!", "Move element", MessageBoxButton.OK, MessageBoxImage.Error);

                        this.OnPropertyChanged("Contents");
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
                    if (ContentWindow.SelectedElements.Count != 1)
                        return;

                    MContentElement elem = ContentWindow.SelectedElements[0];
                    MessageBoxResult res = MessageBox.Show("Are you sure that you want to delete '" + elem.Name + "'?", "Delete element", MessageBoxButton.YesNo, MessageBoxImage.Question);
                    if (res == MessageBoxResult.Yes)
                    {
                        if (!this.ContentManager.DeleteElement(elem.ID))
                            MessageBox.Show("Cannot delete content element '" + elem.Name + "'!", "Delete element", MessageBoxButton.OK, MessageBoxImage.Error);

                        ContentWindow.SelectedElements.Clear();
                        this.OnPropertyChanged("Contents");
                    }
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

                    throw new System.NotImplementedException();
                });
            }
        }


        // Add content element commands
        public ICommand AddMeshElementCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    throw new System.NotImplementedException();
                });
            }
        }

        #endregion


        public ContentWindow(MContentManager contentManager)
        {
            InitializeComponent();
            this.DataContext = this;

            this.ContentManager = contentManager;

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
                ContentWindow.SelectedElements.Remove(item.Element);

            foreach (ContentItem item in e.AddedItems)
                ContentWindow.SelectedElements.Add(item.Element);
        }

        // TODO: drop
        // TODO: move/copy by drag
        // TODO: preview


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
			this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }

    }
}
