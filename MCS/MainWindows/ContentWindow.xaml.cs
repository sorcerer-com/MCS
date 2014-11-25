using MCS.Managers;
using MEngine;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Input;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for ContentWindow.xaml
    /// </summary>
    public partial class ContentWindow : Window, INotifyPropertyChanged
    {
        public MContentManager ContentManager { get; private set; }


        #region Helpers Structures

        public struct TreeItem
        {
            public string FullPath { get; private set; }
            public string Image { get; private set; }
            public Dictionary<string, TreeItem> Children { get; private set; }

            public TreeItem(string fullPath, string image) : this()
            {
                this.FullPath = fullPath;
                this.Image = image;
                this.Children = new Dictionary<string, TreeItem>();
            }
        }

        public struct ContentItem
        {
            public string Image { get; private set; }
            public MContentElement Element { get; private set; }

            public ContentItem(string image, MContentElement element) : this()
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
                        result.Add(package, new TreeItem(package + "#", "/Images/ContentWindow/package.png"));
                    
                    TreeItem curr = result[package];
                    string[] folders = path.Split(new char[] { '\\' }, System.StringSplitOptions.RemoveEmptyEntries);
                    foreach(string folder in folders)
                    {
                        if (!curr.Children.ContainsKey(folder))
                            curr.Children.Add(folder, new TreeItem(curr.FullPath + folder + "\\", "/Images/ContentWindow/folder.png"));
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
                ObservableCollection<ContentItem> result =
                    new ObservableCollection<ContentItem>();

                List<MContentElement> elements = this.ContentManager.Content;
                foreach(MContentElement element in elements)
                {
                    if (!element.FullName.ToLowerInvariant().Contains(this.Filter) &&
                        !element.ID.ToString().ToLowerInvariant().Contains(this.Filter) &&
                        !element.Type.ToString().ToLowerInvariant().Contains(this.Filter))
                        continue;

                    if (!string.IsNullOrEmpty(this.SelectedFullPath) && !element.FullPath.Equals(this.SelectedFullPath))
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

                return result;
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

        private string selectedFullPath;
        public string SelectedFullPath
        {
            get { return this.selectedFullPath; }
            set
            {
                this.selectedFullPath = value;
                this.OnPropertyChanged("SelectedFullPath");
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
                        this.SelectedFullPath = string.Empty;
                        this.Filter = string.Empty;
                    });
            }
        }


        public ICommand CreatePackageCommand
        {
            get
            {
                return new DelegateCommand((o) =>
                {
                    MessageBox.Show("test");
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
            this.selectedFullPath = string.Empty;
        }


        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            KeyValuePair<string, TreeItem> selectedItem = (KeyValuePair<string, TreeItem>)e.NewValue;
            this.SelectedFullPath = selectedItem.Value.FullPath;
        }

        // TODO: drop


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
			this.PropertyChanged(this, new PropertyChangedEventArgs(info));
        }
    }
}
