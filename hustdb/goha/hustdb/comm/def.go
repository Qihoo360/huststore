package comm

type HustdbResponse struct {
	Data    []byte
	Code    int
	Version int
	Backend string
}
