package raft

// ServerID is a unique string identifying a server for all time.
type ServerID string

// ServerAddress is a network address for a server that a transport can contact.
type ServerAddress string

// whether a Server in a Configuration gets a vote.
type ServerSuffrage int

const (
	Voter ServerSuffrage = iota
	// Nonvoter is a server that receives log entries but is not considered for
	// elections or commitment purposes.
	Nonvoter
	// Staging is a server that acts like a nonvoter with one exception: once a
	// staging server receives enough log entries to be sufficiently caught up to
	// the leader's log, the leader will invoke a  membership change to change
	// the Staging server to a Voter.
	Staging
)

func (this *ServerSuffrage) String() string {
	switch *this {
	case Voter:
		return "Voter"
	case Nonvoter:
		return "Nonvoter"
	case Staging:
		return "Staging"
	}
	return "ServerSuffrage"
}

type Server struct {
	Suffrage ServerSuffrage
	ID       ServerID
	Address  ServerAddress
}
