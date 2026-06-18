pragma Singleton
import QtQuick

QtObject {
    id: root

    enum Types {
        Unknown,
        Song,
        Album,
        Playlist
    }

    function stringMediaTypeToEnum (stringMediaType) {
        switch (stringMediaType) {
            case "Song":
                return MediaTypes.Song;
            case "Album":
                return MediaTypes.Album;
            case "Playlist":
                return MediaTypes.Playlist;
            default:
                return MediaTypes.Unknown;
        }
    }

    function mediaTypeEnumToString (mediaTypeValue) {
        switch (mediaTypeValue) {
            case MediaTypes.Song:
                return "Song";
            case MediaTypes.Album:
                return "Album";
            case MediaTypes.Playlist:
                return "Playlist";
            default:
                return "Unknown";
        }
    }
}