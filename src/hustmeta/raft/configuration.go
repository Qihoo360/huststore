package raft

import "fmt"

// The servers are listed no particular order, but each should only appear once.
// These entries are appended to the log during membership changes.
type Configuration struct {
	Servers []Server
}

func (this *Configuration) Clone() *Configuration {
	other := &Configuration{}
	other.Servers = append(other.Servers, this.Servers...)
	return other
}

func (this *Configuration) HasVote(id ServerID) bool {
	for _, server := range this.Servers {
		if server.ID == id {
			return server.Suffrage == Voter
		}
	}
	return false
}

func (this *Configuration) Check() error {
	idSet := map[ServerID]bool{}
	addressSet := map[ServerAddress]bool{}
	var voters int
	for _, server := range this.Servers {
		if len(server.ID) < 1 {
			return fmt.Errorf("Empty ID in configuration: %v", this)
		}
		if len(server.Address) < 1 {
			return fmt.Errorf("Empty address in configuration: %v", server)
		}
		if _, ok := idSet[server.ID]; ok {
			return fmt.Errorf("Found duplicate ID in configuration: %v", server.ID)
		}
		idSet[server.ID] = true
		if _, ok := addressSet[server.Address]; ok {
			return fmt.Errorf("Found duplicate address in configuration: %v", server.Address)
		}
		addressSet[server.Address] = true
		if server.Suffrage == Voter {
			voters++
		}
	}
	if voters < 1 {
		return fmt.Errorf("Need at least one voter in configuration: %v", this)
	}
	return nil
}

func (this *Configuration) indexof(serverID ServerID) int {
	for i, server := range this.Servers {
		if server.ID == serverID {
			return i
		}
	}
	return -1
}

func (this *Configuration) addServer(index int, suffrage ServerSuffrage, request *configurationChangeRequest) {
	newServer := Server{
		Suffrage: suffrage,
		ID:       request.serverID,
		Address:  request.serverAddress,
	}
	if index < 0 {
		this.Servers = append(this.Servers, newServer)
	} else {
		if Voter == this.Servers[index].Suffrage {
			this.Servers[index].Address = request.serverAddress
		} else {
			this.Servers[index] = newServer
		}
	}
}

func (this *Configuration) Next(currentIndex uint64, request *configurationChangeRequest) (*Configuration, error) {
	if request.prevIndex > 0 && request.prevIndex != currentIndex {
		return nil, fmt.Errorf("Configuration changed since %v (latest is %v)", request.prevIndex, currentIndex)
	}
	index := this.indexof(request.serverID)
	configuration := this.Clone()
	switch request.command {
	case AddStaging:
		this.addServer(index, Voter, request)
	case AddNonvoter:
		this.addServer(index, Nonvoter, request)
	case DemoteVoter:
		this.Servers[index].Suffrage = Nonvoter
	case RemoveServer:
		this.Servers = append(this.Servers[:index], this.Servers[index+1:]...)
	case Promote:
		if Staging == this.Servers[index].Suffrage {
			this.Servers[index].Suffrage = Voter
		}
	}
	if err := configuration.Check(); nil != err {
		return nil, err
	}
	return configuration, nil
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

// configurationChangeRequest describes a change that a leader would like to
// make to its current configuration. It's used only within a single server
// (never serialized into the log), as part of `configurationChangeFuture`.
type configurationChangeRequest struct {
	command ConfigurationChangeCommand

	serverID ServerID

	// only present for AddStaging, AddNonvoter
	serverAddress ServerAddress

	// prevIndex, if nonzero, is the index of the only configuration upon which
	// this change may be applied; if another configuration entry has been
	// added in the meantime, this request will fail.
	prevIndex uint64
}

// there can be at most one uncommitted configuration at a time
// (the next configuration may not be created until the prior one has been committed).
type configurations struct {
	committed      Configuration
	committedIndex uint64
	// latest is the latest configuration in the log/snapshot (may be committed or uncommitted)
	latest      Configuration
	latestIndex uint64
}

func (this *configurations) Clone() *configurations {
	return &configurations{
		committed:      *this.committed.Clone(),
		committedIndex: this.committedIndex,
		latest:         *this.latest.Clone(),
		latestIndex:    this.latestIndex,
	}
}
