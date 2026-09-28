# Tables

## songs

| Field Name     | Type         | Nullable? | Primary Key? | Notes                                                 |
|----------------|--------------|-----------|--------------|-------------------------------------------------------|
| song_uuid      | uuid         | No        | Yes          |                                                       |
| artist_uuid    | uuid         | No        | No           |                                                       |
| album_uuid     | uuid         | Yes       | No           | NULL if the song was never included on an album       |
| name           | varchar(100) | No        | No           | Song name                                             |
| track_number   | int16        | No        | No           |                                                       |
| favourite      | boolean      | No        | No           | Default value false                                   |
| shazam_id      | varchar(100) | Yes       | No           | Shazam's own track ID (the "key" field), for dedup    |
| isrc           | varchar(100) | Yes       | No           | International Standard Recording Code, if known       |

### Indexes

* album_id  - For searching by album 
* artist_id - For searching by artist
* shazam_id - For deduping/looking up by Shazam's track ID

## history

| Field Name     | Type         | Nullable? | Primary Key? | Notes                                                 |
|----------------|--------------|-----------|--------------|-------------------------------------------------------|
| identified_on  | datetime     | No        | Yes          | Timestamp this detection occurred                     |
| song_uuid      | uuid         | No        | Yes          | References songs.song_uuid                            |

### Indexes

* song_uuid      - For looking up a song's detection history
* identified_on  - For sorting/filtering by detection date

## artists

| Field Name     | Type         | Nullable? | Primary Key? | Notes                                                 |
|----------------|--------------|-----------|--------------|-------------------------------------------------------|
| artist_uuid    | uuid         | No        | Yes          |                                                       |
| name           | varchar(100) | No        | No           | Song name                                             |
| website        | varchar(100) | Yes       | No           |                                                       |
| bandcamp       | varchar(100) | Yes       | No           |                                                       |
| discogs        | varchar(100) | Yes       | No           |                                                       |
| favourite      | boolean      | No        | No           | Default value false                                   |

### Indexes

* name

## albums

| Field Name   | Type         | Nullable? | Primary Key? | Notes                                                 |
|--------------|--------------|-----------|--------------|-------------------------------------------------------|
| album_uuid   | uuid         | No        | Yes          |                                                       |
| name         | varchar(100) | No        | No           | Song name                                             |
| release_date | datetime     | No        | No           |                                                       |
| label_uuid   | uuid         | No        | No           |                                                       |
| website      | varchar(100) | Yes       | No           |                                                       |
| bandcamp     | varchar(100) | Yes       | No           |                                                       |
| discogs      | varchar(100) | Yes       | No           |                                                       |
| favourite    | boolean      | No        | No           | Default value false                                   |

### Indexes

* name
* release_date
* label_uuid

## labels

| Field Name   | Type         | Nullable? | Primary Key? | Notes                                                 |
|--------------|--------------|-----------|--------------|-------------------------------------------------------|
| label_uuid   | uuid         | No        | Yes          |                                                       |
| name         | varchar(100) | No        | No           | Song name                                             |
