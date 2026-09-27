# Tables

## songs

| Field Name   | Type         | Nullable? | Primary Key? | Notes                                                 |
|--------------|--------------|-----------|--------------|-------------------------------------------------------|
| song_uuid    | uuid         | No        | Yes          |                                                       |
| artist_uuid  | uuid         | No        | No           |                                                       |
| album_uuid   | uuid         | Yes       | No           | NULL if the song was never included on an album       |
| name         | varchar(100) | No        | No           | Song name                                             |
| track_number | int16        | No        | No           |                                                       |
| count        | int16        | No        | No           | Number of times this song was identified. Always >= 1 |
| favourite    | boolean      | No        | No           | Default value false                                   |

### Indexes

* album_id  - For searching by album 
* artist_id - For searching by artist

## artists

| Field Name   | Type         | Nullable? | Primary Key? | Notes                                                 |
|--------------|--------------|-----------|--------------|-------------------------------------------------------|
| artist_uuid  | uuid         | No        | Yes          |                                                       |
| name         | varchar(100) | No        | No           | Song name                                             |
| website      | varchar(100) | Yes       | No           |                                                       |
| bandcamp     | varchar(100) | Yes       | No           |                                                       |
| discogs      | varchar(100) | Yes       | No           |                                                       |
| favourite    | boolean      | No        | No           | Default value false                                   |

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
