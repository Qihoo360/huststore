package raft

// The servers are listed no particular order, but each should only appear once.
// These entries are appended to the log during membership changes.
type Configuration struct {
	Servers []Server
}

func (this *Configuration) Clone(other Configuration) {
	other.Servers = append(other.Servers, this.Servers...)
	return
}

type ConfigurationChangeCommand uint8

const (
	// makes a server Staging unless its Voter.
	AddStaging ConfigurationChangeCommand = iota
	// makes a server Nonvoter unless its Staging or Voter.
	AddNonvoter
	// makes a server Nonvoter unless its absent.
	DemoteVoter
	// removes a server entirely from the cluster membership.
	RemoveServer
	// created by leader; it turns a Staging server into a Voter
	Promote
)

func (this *ConfigurationChangeCommand) String() string {
	switch *this {
	case AddStaging:
		return "AddStaging"
	case AddNonvoter:
		return "AddNonvoter"
	case DemoteVoter:
		return "DemoteVoter"
	case RemoveServer:
		return "RemoveServer"
	case Promote:
		return "Promote"
	}
	return "ConfigurationChangeCommand"
}
