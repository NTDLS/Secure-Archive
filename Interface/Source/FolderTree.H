#ifndef _FolderTree_H
#define _FolderTree_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOAD_TREE_ITEMS_INCREMENT 10

#define TREE_IMAGE_UNKNOWN		 0
#define TREE_IMAGE_REMOVABLE	 1
#define TREE_IMAGE_FIXED		 2
#define TREE_IMAGE_NETWORK		 3
#define TREE_IMAGE_CDROM		 4
#define TREE_IMAGE_RAMDRIVE		 5
#define TREE_IMAGE_USB			 6
#define TREE_IMAGE_DESKTOP		 7
#define TREE_IMAGE_MYDOCS		 8
#define TREE_IMAGE_CLOSEDFOLDER	 9
#define TREE_IMAGE_OPENFOLDER	 10
#define TREE_IMAGE_MYCOMPUTER	 11
#define TREE_IMAGE_MYDOCUMENTS	 12
#define TREE_IMAGE_MYPICTURES	 13
#define TREE_IMAGE_MYMOVIES		 14
#define TREE_IMAGE_MYMUSIC		 15
#define TREE_IMAGE_SECUREARCHIVE 16

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FolderTree {
    public:
        BOOL FolderTreeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        bool InitFolderTree(HWND hTreeHandle, HWND hButtonHandle, HWND hSaveToLocation, HWND hSaveToFile);
        bool UnInitFolderTree(void);
        bool SelectPath(const char *sINPath);
		BOOL DeleteAllItems(void);

    private:
		HTREEITEM *LoadedList;

		HTREEITEM hDesktopLeaf;
		HTREEITEM hMyDocumentsLeaf;
		HTREEITEM hMyComputerLeaf;

        char sDesktopPath[MAX_PATH];
		char sMyDocuments[MAX_PATH];
        char gsExtractLocation[MAX_PATH];

        HIMAGELIST hImageList;

        HWND lhButtonHandle;
        HWND lhSaveToLocation;
		HWND lhSaveToFile;
        HWND lhTreeHandle;

        int iLoadedItems;
        int iMaxLoadedItems;

		bool bShowHiddenFiles;
		bool bShowSuperHiddenFiles;

		int ChildPathMatchLength(const char *sInParent, const char *sInChild);
		int GetPathTok(const char *sFullPath, char *sOutTok, int *iInOutRPos);
		HTREEITEM FindImmediateChild(HTREEITEM hParent, const char *sText);

		bool CreateTreeImageList(void);
        bool FreeAlreadyLoaded(void);
        BOOL GetTreeItemText(HTREEITEM htItem, char *sOUTText);
		int GetTreeItemImage(HTREEITEM htItem);
		bool IndexDirectoryForSubItem(HTREEITEM hSubLeaf, const char *sINFolder, bool bIndexTwoDeep, bool bIgnoreAlreadyLoaded);
        bool InitAlreadyLoaded(void);
        bool IsAlreadyLoaded(HTREEITEM hSubLeaf);
        bool PopBrowseTree(void);
        HTREEITEM AddTreeViewItem(HTREEITEM hParent, char *sINText, int iImage, int iSelImage);
		void SetEditCursorToEnd(void);
		STORAGE_BUS_TYPE GetBusType(const char *sDisk);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

